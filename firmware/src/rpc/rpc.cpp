#include <NimBLEDevice.h>

#include <etl/pool.h>

#include "api.hpp"
#include "components/prefs.hpp"
#include "logger.hpp"
#include "rpc.hpp"
#include "session.hpp"

RpcDispatcher rpc;

namespace {

// UUIDs for the BLE service and characteristic
const char* SERVICE_UUID = "5f524546-4c4f-575f-5250-435f5356435f"; // _REFLOW_RPC_SVC_
const char* RPC_CHARACTERISTIC_UUID = "5f524546-4c4f-575f-5250-435f494f5f5f"; // _REFLOW_RPC_IO__

using ConnHandle = Session::ConnHandle;
using SessionPool = etl::pool<Session, MYNEWT_VAL(BLE_MAX_CONNECTIONS)>;

auto bleNameStore = AsyncPreference<BleName>(PrefsWriter::getInstance(), AsyncPreferenceKV::getInstance(), PREFS_NAMESPACE, "ble_name", BleName{"Reflow Table"});

SessionPool session_pool;

auto find_session(ConnHandle conn_handle) -> Session* {
    for (auto it = session_pool.begin(); it != session_pool.end(); ++it) {
        Session& session = it.get<Session>();
        if (session.conn_handle == conn_handle) { return &session; }
    }

    return nullptr;
}

void remove_session(ConnHandle conn_handle) {
    Session* session = find_session(conn_handle);
    if (session == nullptr) { return; }
    session_pool.destroy(session);
}

void clear_stale_sessions(NimBLEServer& server) {
    for (auto it = session_pool.begin(); it != session_pool.end();) {
        Session& session = it.get<Session>();
        if (server.getPeerInfoByHandle(session.conn_handle).getConnHandle() == session.conn_handle) {
            ++it;
        } else {
            APP_LOGI("BLE: stale session found, removing, conn_handle {}", session.conn_handle);
            auto stale = it++;
            session_pool.destroy(&stale.get<Session>());
        }
    }
}

auto ensure_session(ConnHandle conn_handle) -> Session* {
    if (Session* session = find_session(conn_handle)) {
        return session;
    }

    Session* session = session_pool.create(conn_handle);
    if (session == nullptr) {
        APP_LOGE("BLE: session create failed, allocation failed, conn_handle {}", conn_handle);
        return nullptr;
    }

    return session;
}

class RpcCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        ConnHandle conn_handle = connInfo.getConnHandle();
        Session* session = find_session(conn_handle);
        if (session == nullptr) {
            APP_LOGE("BLE: missing session on write, conn_handle {}", conn_handle);
            return;
        }

        const auto& chunk = pCharacteristic->getValue();
        session->consumeChunk(chunk.data(), chunk.size());
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        ConnHandle conn_handle = connInfo.getConnHandle();
        Session* session = find_session(conn_handle);
        if (session == nullptr) {
            APP_LOGE("BLE: missing session on read, conn_handle {}", conn_handle);
            return;
        }

        const auto chunk = session->getResponseChunk();
        pCharacteristic->setValue(chunk.data, chunk.size);
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        auto conn_handle = connInfo.getConnHandle();
        clear_stale_sessions(*pServer);

        // Always restart advertising to allow multiple clients
        NimBLEDevice::startAdvertising();

        APP_LOGI("BLE: Device connected, conn_handle {} (total {} of {})",
            conn_handle, pServer->getConnectedCount(), MYNEWT_VAL(BLE_MAX_CONNECTIONS));

        if (ensure_session(conn_handle) == nullptr) {
            pServer->disconnect(conn_handle);
            return;
        }

        // For BLE 5 clients with DLE extension support - set data packet size to max.
        // This boosts transfer speed to ~ 45 Kb/sec for big transfers.
        pServer->setDataLen(conn_handle, 251);

        // Update connection parameters for maximum performance and stability
        // min conn interval 7.5ms, max conn interval 7.5ms, latency 0, timeout 2s
        // Smaller intervals help with BLE 4 clients without DLE, but makes no sense
        // for BLE 5 clients with DLE.
        pServer->updateConnParams(conn_handle, 0x06, 0x06, 0, 200);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        auto conn_handle = connInfo.getConnHandle();
        remove_session(conn_handle);
        APP_LOGI("BLE: Device disconnected, conn_handle {}, reason {} (total {} of {})",
            conn_handle, reason, pServer->getConnectedCount(), MYNEWT_VAL(BLE_MAX_CONNECTIONS));
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
} // namespace

void ble_name_write(const BleName& name) {
    bleNameStore.set(name);

    const std::string nimble_name(name.data(), name.size());
    NimBLEDevice::setDeviceName(nimble_name);

    auto adv = NimBLEDevice::getAdvertising();
    adv->setName(nimble_name);
    adv->refreshAdvertisingData();
}

const BleName& ble_name_read() {
    return bleNameStore.get();
}

void rpc_start() {
    api_methods_create(rpc);
    ble_init();
}
