#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>
#include <memory>
#include "logger.hpp"
#include "rpc.hpp"
#include "lib/ble_chunker.hpp"
#include "components/prefs.hpp"
#include "ble_auth_store.hpp"
#include "auth_utils.hpp"
#include "app.hpp"
#include "api.hpp"

MsgpackRpcDispatcher rpc;
MsgpackRpcDispatcher auth_rpc;

namespace {

bool pairing_enabled_flag = false;

auto is_pairing_enabled() -> bool { return pairing_enabled_flag; }

// UUIDs for the BLE service and characteristic
const char* SERVICE_UUID = "5f524546-4c4f-575f-5250-435f5356435f"; // _REFLOW_RPC_SVC_
const char* RPC_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f494f5f5f"; // _REFLOW_RPC_IO__
const char* AUTH_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f41555448"; // _REFLOW_RPC_AUTH

auto bleAuthStore = BleAuthStore<4>(PrefsWriter::getInstance(), AsyncPreferenceKV::getInstance(), PREFS_NAMESPACE);
auto bleNameStore = AsyncPreference<std::string>(PrefsWriter::getInstance(), AsyncPreferenceKV::getInstance(), PREFS_NAMESPACE, "ble_name", "Reflow Table");

class Session;

std::map<decltype(ble_gap_conn_desc::conn_handle), std::shared_ptr<Session>> sessions;
std::shared_ptr<Session> context;

void set_context(std::shared_ptr<Session> ctx) { context = ctx; }
auto get_context() -> std::shared_ptr<Session> { return context; }

class Session : public std::enable_shared_from_this<Session> {
public:
    Session()
        : rpcChunker(16*1024 + 500), authChunker(1*1024), authenticated(false)
    {
        rpcChunker.onMessage = [this](const std::vector<uint8_t>& message) {
            DEBUG("Free memory: {}; Minimum free memory: {}; Max free block: {}",
                heap_caps_get_free_size(MALLOC_CAP_8BIT),
                heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

            DEBUG("BLE: Received message of length {}", message.size());

            if (authenticated) {
                std::vector<uint8_t> response;
                rpc.dispatch(message, response);
                return response;
            }

            const auto doc = msgpack_rpc_dispatcher::create_response(false, "Not authenticated");
            std::vector<uint8_t> error;
            msgpack_rpc_dispatcher::serialize_to(doc, error);
            return error;
        };

        authChunker.onMessage = [this](const std::vector<uint8_t>& message) {
            set_context(shared_from_this());

            std::vector<uint8_t> response;
            auth_rpc.dispatch(message, response);

            set_context(nullptr);
            return response;
        };

        // No default value allowed!
        random = create_secret();
    }
    ~Session() { DEBUG("BLE: Session destroyed"); }

    BleChunker rpcChunker;
    BleChunker authChunker;
    bool authenticated;
    std::array<uint8_t, 32> random;
};

class RpcCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) { return; }

        auto session = sessions[conn_handle];
        session->rpcChunker.consumeChunk(pCharacteristic->getValue(), pCharacteristic->getDataLength());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) { return; }
        pCharacteristic->setValue(sessions[conn_handle]->rpcChunker.getResponseChunk());
    }
};

class AuthCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) { return; }
        DEBUG("BLE AUTH: Received chunk of length {}", pCharacteristic->getDataLength());
        const auto& sec = desc->sec_state;
        DEBUG("BLE AUTH security state: encrypted {}, authenticated {}, bonded {}",
            sec.encrypted, sec.authenticated, sec.bonded);

        auto session = sessions[conn_handle];
        session->authChunker.consumeChunk(pCharacteristic->getValue(), pCharacteristic->getDataLength());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        auto conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) { return; }
        pCharacteristic->setValue(sessions[conn_handle]->authChunker.getResponseChunk());
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
        auto conn_handle = desc->conn_handle;
        sessions[conn_handle] = std::make_shared<Session>();
        DEBUG("BLE: Device connected, conn_handle {}", conn_handle);

        // For BLE 5 clients with DLE extension support - set data packet size to max.
        // This boosts transfer speed to ~ 45 Kb/sec for big transfers.
        pServer->setDataLen(conn_handle, 251);

        // Update connection parameters for maximum performance and stability
        // min conn interval 7.5ms, max conn interval 7.5ms, latency 0, timeout 2s
        // Smaller intervals help with BLE 4 clients without DLE, but makes no sense
        // for BLE 5 clients with DLE.
        pServer->updateConnParams(conn_handle, 0x06, 0x06, 0, 200);
    }

    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
        auto conn_handle = desc->conn_handle;
        DEBUG("BLE: Device disconnected, conn_handle {}", conn_handle);
        sessions.erase(conn_handle);
    }

    void onMTUChange(uint16_t mtu, ble_gap_conn_desc* desc) override {
        DEBUG("BLE: MTU updated to {}, conn_handle {}", mtu, desc->conn_handle);
    }

    // Used for testing purposes, to check if encryption is working
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        const auto& sec = desc->sec_state;
        DEBUG("BLE: Authentication complete, conn_handle {}, encrypted {}, authenticated {}, bonded {}",
            desc->conn_handle, sec.encrypted, sec.authenticated, sec.bonded);
    }
};


void ble_init() {
    const std::string name = bleNameStore.get().substr(0, 20); // Limit name length
    NimBLEDevice::init(name);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Set the power level to maximum
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

    // Setup RPC service and characteristic.
    NimBLEService* service = server->createService(SERVICE_UUID);

    NimBLECharacteristic* rpc_characteristic = service->createCharacteristic(
        RPC_CHARACTERISTIC_UUID,
        // This hangs comm if enabled. Seems Web BT not supports encryption
        //NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    rpc_characteristic->setCallbacks(new RpcCharacteristicCallbacks());

    NimBLECharacteristic* auth_characteristic = service->createCharacteristic(
        AUTH_CHARACTERISTIC_UUID,
        // This hangs comm if enabled. Seems Web BT not supports encryption
        //NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    auth_characteristic->setCallbacks(new AuthCharacteristicCallbacks());

    service->start();

    // Configure advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    NimBLEDevice::startAdvertising();

    DEBUG("BLE initialized");
}

std::vector<uint8_t> auth_info() {
    auto session = get_context();

    session->random = create_secret(); // renew hmac msg every time!

    JsonDocument doc;
    auto mac = get_own_mac();
    doc["id"] = MsgPackBinary(mac.data(), mac.size());
    doc["hmac_msg"] = MsgPackBinary(session->random.data(), session->random.size());
    doc["pairable"] = is_pairing_enabled();

    size_t size = measureMsgPack(doc);
    std::vector<uint8_t> output(size);
    serializeMsgPack(doc, output.data(), size);
    return output;
}

auto authenticate(const std::vector<uint8_t> client_id,
                 const std::vector<uint8_t> hmac,
                 uint64_t timestamp) -> bool {
    auto session = get_context();

    BleAuthId auth_id;
    std::copy(client_id.begin(), client_id.end(), auth_id.data());

    auto random = session->random;
    session->random = create_secret();

    if (!bleAuthStore.has(auth_id)) { return false; }

    BleAuthSecret secret;
    bleAuthStore.get_secret(auth_id, secret);

    auto hmac_expected = hmac_sha256(random, secret);
    if (!std::equal(hmac.begin(), hmac.end(), hmac_expected.begin(), hmac_expected.end())) {
        return false;
    }

    session->authenticated = true;
    bleAuthStore.set_timestamp(auth_id, timestamp);
    return true;
}

auto pair(const std::vector<uint8_t> client_id) -> std::vector<uint8_t> {
    if (!is_pairing_enabled()) {
        return std::vector<uint8_t>{};
    }

    BleAuthId auth_id;
    std::copy(client_id.begin(), client_id.end(), auth_id.data());

    auto secret = create_secret();
    bleAuthStore.create(auth_id, secret);

    application.receive(AppCmd::BondOff());

    return std::vector<uint8_t>(secret.begin(), secret.end());
}

} // namespace

void pairing_enable() { pairing_enabled_flag = true; }
void pairing_disable() { pairing_enabled_flag = false; }

void rpc_start() {
    auth_rpc.addMethod("auth_info", auth_info);
    auth_rpc.addMethod("authenticate", authenticate);
    auth_rpc.addMethod("pair", pair);

    api_methods_create();
    ble_init();
}
