#include <NimBLEDevice.h>

#include <algorithm>
#include <array>
#include <map>
#include <memory>

#include <cbor.h>
#include <etl/string.h>
#include <etl/vector.h>

#include "api.hpp"
#include "app.hpp"
#include "auth_utils.hpp"
#include "ble_auth_store.hpp"
#include "components/prefs.hpp"
#include "lib/ble_chunker.hpp"
#include "logger.hpp"
#include "proto/generated/shared_constants.hpp"
#include "rpc.hpp"

RpcDispatcher rpc;

namespace {

using AuthBuffer = etl::vector<uint8_t, SharedConstants::MAX_RPC_MESSAGE_SIZE>;
using RpcChunker = BleChunker<SharedConstants::MAX_RPC_MESSAGE_SIZE>;
using AuthParams = cbor_rpc_dispatcher::ParamsReader;
using AuthResponse = RpcDispatcher::Response;

bool pairing_enabled_flag = false;

auto is_pairing_enabled() -> bool { return pairing_enabled_flag; }

// UUIDs for the BLE service and characteristic
const char* SERVICE_UUID = "5f524546-4c4f-575f-5250-435f5356435f"; // _REFLOW_RPC_SVC_
const char* RPC_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f494f5f5f"; // _REFLOW_RPC_IO__

auto bleAuthStore = BleAuthStore<4>(PrefsWriter::getInstance(), AsyncPreferenceKV::getInstance(), PREFS_NAMESPACE);
auto bleNameStore = AsyncPreference<BleName>(PrefsWriter::getInstance(), AsyncPreferenceKV::getInstance(), PREFS_NAMESPACE, "ble_name", BleName{"Reflow Table"});

class Session;

std::map<decltype(ble_gap_conn_desc::conn_handle), std::shared_ptr<Session>> sessions;
std::shared_ptr<Session> context;

void set_context(std::shared_ptr<Session> ctx) { context = std::move(ctx); }
auto get_context() -> const std::shared_ptr<Session>& { return context; }

class Session : public std::enable_shared_from_this<Session> {
public:
    Session() {
        rpcChunker.setMessageHandler(RpcChunker::MessageHandler::create<Session, &Session::handleRpcMessage>(*this));
        // No default value allowed!
        random = create_secret();
    }

    ~Session() { APP_LOGI("BLE: Session destroyed"); }

    void handleRpcMessage(const RpcChunker::MessageBuffer& message, RpcChunker::MessageBuffer& response) {
        /*APP_LOGI("Free memory: {}; Minimum free memory: {}; Max free block: {}",
            heap_caps_get_free_size(MALLOC_CAP_8BIT),
            heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

        APP_LOGI("BLE: Received message of length {}", message.size());*/
        set_context(shared_from_this());

        if (!authenticated && rpc.needs_authentication(message)) {
            RpcDispatcher::Response writer(response);
            writer.write_error("Not authenticated");
            set_context(nullptr);
            return;
        }

        rpc.dispatch(message, response);
        set_context(nullptr);
    }

    RpcChunker rpcChunker;
    bool authenticated{false};
    std::array<uint8_t, 32> random{};
};

class RpcCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        uint16_t conn_handle = connInfo.getConnHandle();
        if (!sessions.count(conn_handle)) { return; }

        auto session = sessions[conn_handle];
        const std::vector<uint8_t> chunk = pCharacteristic->getValue();
        session->rpcChunker.consumeChunk(chunk.data(), chunk.size());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        uint16_t conn_handle = connInfo.getConnHandle();
        if (!sessions.count(conn_handle)) { return; }

        const auto chunk = sessions[conn_handle]->rpcChunker.getResponseChunk();
        pCharacteristic->setValue(chunk.data, chunk.size);
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        auto conn_handle = connInfo.getConnHandle();
        sessions[conn_handle] = std::make_shared<Session>();
        APP_LOGI("BLE: Device connected, conn_handle {}", conn_handle);

        // For BLE 5 clients with DLE extension support - set data packet size to max.
        // This boosts transfer speed to ~ 45 Kb/sec for big transfers.
        pServer->setDataLen(conn_handle, 251);

        // Update connection parameters for maximum performance and stability
        // min conn interval 7.5ms, max conn interval 7.5ms, latency 0, timeout 2s
        // Smaller intervals help with BLE 4 clients without DLE, but makes no sense
        // for BLE 5 clients with DLE.
        pServer->updateConnParams(conn_handle, 0x06, 0x06, 0, 200);

        // Keep advertising active until we reach the connection limit.
        if (pServer->getConnectedCount() < NIMBLE_MAX_CONNECTIONS) {
            NimBLEDevice::startAdvertising();
            APP_LOGI("BLE: Advertising restarted, connected {}/{}",
                pServer->getConnectedCount(), NIMBLE_MAX_CONNECTIONS);
        }
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        auto conn_handle = connInfo.getConnHandle();
        APP_LOGI("BLE: Device disconnected, conn_handle {}", conn_handle);
        sessions.erase(conn_handle);
    }

    void onMTUChange(uint16_t mtu, NimBLEConnInfo& connInfo) override {
        APP_LOGI("BLE: MTU updated to {}, conn_handle {}", mtu, connInfo.getConnHandle());
    }

    // Used for testing purposes, to check if encryption is working
    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        APP_LOGI("BLE: Authentication complete, conn_handle {}, encrypted {}, authenticated {}, bonded {}",
            connInfo.getConnHandle(), connInfo.isEncrypted(),
            connInfo.isEncrypted(), connInfo.isBonded());
    }
};


void ble_init() {
    const auto& name = bleNameStore.get();
    const std::string nimble_name(name.data(), name.size());
    // This name is used in Device Name (0x1800/0x2A00), always created by NimBLE
    // But we still need it in advertisement, to show in device selection dialog
    // BEFORE device been connected.
    NimBLEDevice::init(nimble_name);
    NimBLEDevice::setPower(9); // Set the power level to maximum, 9dbm for esp32-c3
    // By default NimBLE already set MTU tu 255. No need to tune it manually.
    // That's enough for 244 bytes read/write to long characteristics.
    // 244 bytes "chunk" - optimal to fit into 1 DLE data packet (251 bytes max).
    // This minimizes overheads and maximizes speed. Alternate value is 495 bytes,
    // (2 DLE data packets) but it gives no notable benefits in real world.
    //
    // NimBLEDevice::setMTU(255);

    // Configure "Just Works" encryption. Notes:
    //
    // 1. Unfortunately, this seems to works only when device attached via
    //    OS "control panel". Web Bluetooth on linux sucks :(.
    // 2. If bonding set to mandatory, browser can't connect until device bonded
    //    via control panel. Since that's not user-friendly, we allow anyone
    //    to connect, and use hand-crafted HMAC-based authentication.
    //
    // So, encryption is not actually used, but exists for memory. May be this
    // will be useful in future.
    NimBLEDevice::setSecurityAuth(false, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());
    server->advertiseOnDisconnect(true);

    // Setup RPC service and characteristic.
    NimBLEService* service = server->createService(SERVICE_UUID);

    NimBLECharacteristic* rpc_characteristic = service->createCharacteristic(
        RPC_CHARACTERISTIC_UUID,
        // This hangs comm if enabled. Seems Web BT not supports encryption
        //NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    rpc_characteristic->setCallbacks(new RpcCharacteristicCallbacks());

    service->start();

    // Configure advertising. Since we should support legacy (v4) clients,
    // we have only "normal" advertising + scan response (31 + 32 bytes),
    // to place data records.
    // SERVICE_UUID + prefs should fit into adv packet. Name will go to scan resp.
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->enableScanResponse(true);
    pAdvertising->setPreferredParams(0x06, 0x06);
    // This name is showed in connection dialog
    pAdvertising->setName(nimble_name);
    NimBLEDevice::startAdvertising();

    APP_LOGI("BLE initialized");
}

auto auth_info_data(AuthBuffer& output) -> bool {
    auto session = get_context();
    session->random = create_secret(); // renew hmac msg every time!

    auto mac = get_own_mac();

    output.clear();
    output.resize(output.max_size());

    CborEncoder encoder;
    CborEncoder map;
    cbor_encoder_init(&encoder, output.data(), output.size(), 0);

    CborError error = cbor_encoder_create_map(&encoder, &map, 3);
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "id");
    if (error == CborNoError) error = cbor_encode_byte_string(&map, mac.data(), mac.size());
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "hmac_msg");
    if (error == CborNoError) error = cbor_encode_byte_string(&map, session->random.data(), session->random.size());
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "pairable");
    if (error == CborNoError) error = cbor_encode_boolean(&map, is_pairing_enabled());
    if (error == CborNoError) error = cbor_encoder_close_container_checked(&encoder, &map);

    if (error != CborNoError) {
        output.clear();
        return false;
    }

    output.resize(cbor_encoder_get_buffer_size(&encoder, output.data()));
    return true;
}

void auth_info(const AuthParams& params, AuthResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

    AuthBuffer output{};
    if (!auth_info_data(output)) {
        response.write_error("Internal error");
        return;
    }

    response.write_binary(output);
}

void authenticate(const AuthParams& params, AuthResponse& response) {
    etl::vector<uint8_t, 64> client_id{};
    etl::vector<uint8_t, 64> hmac{};
    uint64_t timestamp = 0;

    if (!params.has_count(3) ||
        !params.get_binary(0, client_id) ||
        !params.get_binary(1, hmac) ||
        !params.get_uint64(2, timestamp))
    {
        response.write_error("Invalid params");
        return;
    }

    auto session = get_context();

    BleAuthId auth_id;
    if (client_id.size() != auth_id.size() || hmac.size() != session->random.size()) {
        response.write_bool(false);
        return;
    }

    std::copy(client_id.begin(), client_id.end(), auth_id.data());

    auto random = session->random;
    session->random = create_secret();

    if (!bleAuthStore.has(auth_id)) {
        response.write_bool(false);
        return;
    }

    BleAuthSecret secret;
    bleAuthStore.get_secret(auth_id, secret);

    auto hmac_expected = hmac_sha256(random, secret);
    if (!std::equal(hmac.begin(), hmac.end(), hmac_expected.begin(), hmac_expected.end())) {
        response.write_bool(false);
        return;
    }

    session->authenticated = true;
    bleAuthStore.set_timestamp(auth_id, timestamp);
    response.write_bool(true);
}

void pair(const AuthParams& params, AuthResponse& response) {
    etl::vector<uint8_t, 64> client_id{};
    if (!params.has_count(1) || !params.get_binary(0, client_id)) {
        response.write_error("Invalid params");
        return;
    }

    if (!is_pairing_enabled()) {
        static constexpr uint8_t empty = 0;
        response.write_binary(&empty, 0);
        return;
    }

    BleAuthId auth_id;
    if (client_id.size() != auth_id.size()) {
        response.write_error("Invalid client id");
        return;
    }

    std::copy(client_id.begin(), client_id.end(), auth_id.data());

    auto secret = create_secret();
    bleAuthStore.create(auth_id, secret);
    application.enqueue_message(AppCmd::BondOff{});
    response.write_binary(secret.data(), secret.size());
}

} // namespace

void ble_name_write(const BleName& name) {
    bleNameStore.set(name);

    const std::string nimble_name(name.data(), name.size());
    NimBLEDevice::setDeviceName(nimble_name);

    auto adv = NimBLEDevice::getAdvertising();
    adv->setName(nimble_name);
    adv->refreshAdvertisingData();
}

BleName ble_name_read() {
    return bleNameStore.get();
}

void pairing_enable() { pairing_enabled_flag = true; }
void pairing_disable() { pairing_enabled_flag = false; }

void rpc_start() {
    rpc.addMethod("auth_info", RpcDispatcher::MethodHandler::create<auth_info>(), true);
    rpc.addMethod("authenticate", RpcDispatcher::MethodHandler::create<authenticate>(), true);
    rpc.addMethod("pair", RpcDispatcher::MethodHandler::create<pair>(), true);

    api_methods_create();
    ble_init();
}
