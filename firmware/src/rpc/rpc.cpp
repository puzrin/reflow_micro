#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>
#include "logger.hpp"
#include "rpc.hpp"
#include "ble_chunker.hpp"
#include "async_preference/prefs.hpp"
#include "ble_auth_store.hpp"
#include "auth_utils.hpp"
#include "app.hpp"

JsonRpcDispatcher rpc;
JsonRpcDispatcher auth_rpc;

namespace {

bool pairing_enabled_flag = false;

bool is_pairing_enabled() { return pairing_enabled_flag; }

// UUIDs for the BLE service and characteristic
const char* SERVICE_UUID = "5f524546-4c4f-575f-5250-435f5356435f"; // _REFLOW_RPC_SVC_
const char* RPC_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f494f5f5f"; // _REFLOW_RPC_IO__
const char* AUTH_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f41555448"; // _REFLOW_RPC_AUTH

auto bleAuthStore = BleAuthStore<4>(prefsKV);
auto bleNameStore = AsyncPreference<std::string>(prefsKV, "settings", "ble_name", "Reflow Table");

class Session;
Session* context;
void set_context(Session* ctx) { context = ctx; }
Session* get_context() { return context; }

class Session {
public:
    Session()
        : rpcChunker(16*1024 + 500), authChunker(1*1024), authenticated(false)
    {
        rpcChunker.onMessage = [](const std::vector<uint8_t>& message) {
            size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_8BIT);
            size_t minimumFreeMemory = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
            DEBUG("Free memory: {} Minimum free memory: {}", uint32_t(freeMemory), uint32_t(minimumFreeMemory));

            DEBUG("BLE: Received message of length {}", uint32_t(message.size()));

            auto session = get_context();
            if (session && session->authenticated) {
                std::vector<uint8_t> response;
                rpc.dispatch(message, response);
                return response;
            }

            const std::string error_json = R"({"ok": false, "result": "Not authenticated"})";

            const auto doc = jrcpd::create_response(false, "Not authenticated");
            std::vector<uint8_t> error;
            jrcpd::serialize_to(doc, error);
            return error;
        };

        authChunker.onMessage = [](const std::vector<uint8_t>& message) {
            std::vector<uint8_t> response;
            auth_rpc.dispatch(message, response);
            return response;
        };

        // No default value allowed!
        random = create_secret();
    }

    BleChunker rpcChunker;
    BleChunker authChunker;
    bool authenticated;
    std::array<uint8_t, 32> random;
};

std::map<uint16_t, Session*> sessions;

class RpcCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) return;
        //DEBUG("BLE: Received chunk of length {}", uint32_t(pCharacteristic->getDataLength()));

        auto session = sessions[conn_handle];
        set_context(session);
        session->rpcChunker.consumeChunk(pCharacteristic->getValue(), pCharacteristic->getDataLength());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) return;
        //DEBUG("BLE: Reading from characteristic, conn_handle {}", conn_handle);
        pCharacteristic->setValue(sessions[conn_handle]->rpcChunker.getResponseChunk());
    }
};

class AuthCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) return;
        DEBUG("BLE AUTH: Received chunk of length {}", uint32_t(pCharacteristic->getDataLength()));
        const auto& sec = desc->sec_state;
        DEBUG("BLE AUTH security state: encrypted {}, authenticated {}, bonded {}",
            (uint8_t)sec.encrypted, (uint8_t)sec.authenticated, (uint8_t)sec.bonded);

        auto session = sessions[conn_handle];
        set_context(session);
        session->authChunker.consumeChunk(pCharacteristic->getValue(), pCharacteristic->getDataLength());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        if (!sessions.count(conn_handle)) return;
        pCharacteristic->setValue(sessions[conn_handle]->authChunker.getResponseChunk());
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
        uint16_t conn_handle = desc->conn_handle;
        sessions[conn_handle] = new Session();
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
        uint16_t conn_handle = desc->conn_handle;
        DEBUG("BLE: Device disconnected, conn_handle {}", conn_handle);
        delete sessions[conn_handle];
        sessions.erase(conn_handle);
    }

    void onMTUChange(uint16_t mtu, ble_gap_conn_desc* desc) override {
        DEBUG("BLE: MTU updated to {}, conn_handle {}", mtu, desc->conn_handle);
    }

    // Used for testing purposes, to check if encryption is working
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        const auto& sec = desc->sec_state;
        DEBUG("BLE: Authentication complete, conn_handle {}, encrypted {}, authenticated {}, bonded {}",
            desc->conn_handle, (uint8_t)sec.encrypted, (uint8_t)sec.authenticated, (uint8_t)sec.bonded);
    }
};


void ble_init() {
    prefsWriter.add(bleAuthStore);
    prefsWriter.add(bleNameStore);

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

std::string auth_info() {
    auto session = get_context();

    session->random = create_secret(); // renew hmac msg every time!

    JsonDocument doc;
    auto mac = get_own_mac();
    doc["id"] = bin2hex(mac.data(), mac.size());
    doc["hmac_msg"] = bin2hex(session->random.data(), session->random.size());
    doc["pairable"] = is_pairing_enabled();

    std::string output;
    serializeJson(doc, output);
    return output;
}

bool authenticate(const std::string str_client_id, const std::string str_hmac, uint64_t timestamp) {
    auto session = get_context();

    BleAuthId client_id;
    hex2bin(str_client_id, client_id.data(), client_id.size());

    std::array<uint8_t, 32> hmac_response;
    hex2bin(str_hmac, hmac_response.data(), hmac_response.size());

    auto random = session->random;
    session->random = create_secret(); // renew hmac msg every time!

    if (!bleAuthStore.has(client_id)) return false;

    BleAuthSecret secret;
    bleAuthStore.get_secret(client_id, secret);

    auto hmac_expected = hmac_sha256(random, secret);
    if (hmac_response != hmac_expected) return false;

    session->authenticated = true;
    bleAuthStore.set_timestamp(client_id, timestamp);
    return true;
}

std::string pair(const std::string str_client_id) {
    if (!is_pairing_enabled()) return "";

    BleAuthId client_id;
    hex2bin(str_client_id, client_id.data(), client_id.size());

    auto secret = create_secret();
    bleAuthStore.create(client_id, secret);

    // Exit pairing mode on success
    app.receive(BondOff());

    return bin2hex(secret.data(), secret.size());
}

}

void pairing_enable() { pairing_enabled_flag = true; }
void pairing_disable() { pairing_enabled_flag = false; }

void rpc_init() {
    auth_rpc.addMethod("auth_info", auth_info);
    auth_rpc.addMethod("authenticate", authenticate);
    auth_rpc.addMethod("pair", pair);

    ble_init();
}
