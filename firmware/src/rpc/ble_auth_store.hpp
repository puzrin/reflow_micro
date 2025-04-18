#include <array>
#include <algorithm>
#include "lib/async_preference.hpp"

using BleAuthId = std::array<uint8_t, 16>;
using BleAuthSecret = std::array<uint8_t, 32>;

template<size_t MaxRecords = 4>
class BleAuthStore {
public:
    BleAuthStore(IAsyncPreferenceWriter& writer, IAsyncPreferenceKV& kv, const std::string& ns) :
        clientsPref(writer, kv, ns, "ble_clients"),
        timestampsPref(writer, kv, ns, "ble_timestamps") {}

    struct Client {
        BleAuthId id{};
        BleAuthSecret secret{};
    };

    auto has(const BleAuthId& client_id) -> bool {
        return idxById(client_id) != -1;
    }

    auto get_secret(const BleAuthId& client_id, BleAuthSecret& secret_out) -> bool {
        auto idx = idxById(client_id);
        if (idx == -1) { return false; }

        secret_out = clientsPref.get()[idx].secret;
        return true;
    }

    auto set_timestamp(const BleAuthId& client_id, uint64_t timestamp) -> bool {
        auto idx = idxById(client_id);
        if (idx < 0) { return false; }

        auto& timestamps = timestampsPref.get();

        uint64_t current_ts = timestamps[idx];
        constexpr uint64_t one_day_ms = 24 * 60 * 60 * 1000;

        if (timestamp == 0 || timestamp > current_ts + one_day_ms || current_ts > timestamp) {
            timestampsPref.valueUpdateBegin();

            // Update timestamp
            timestamps[idx] = timestamp;

            // Align bad timestamps if any
            for (size_t i = 0; i < MaxRecords; i++) {
                if (timestamps[i] != 0 && timestamps[i] > timestamp + one_day_ms) {
                    timestamps[i] = timestamp;
                }
            }

            timestampsPref.valueUpdateEnd();
        }

        return true;
    }

    auto create(const BleAuthId& client_id, const BleAuthSecret& secret) -> bool {
        auto idx = idxById(client_id);
        if (idx < 0) { idx = idxLRU(); }

        clientsPref.valueUpdateBegin();
        timestampsPref.valueUpdateBegin();

        clientsPref.get()[idx] = { client_id, secret };
        timestampsPref.get()[idx] = 0;

        clientsPref.valueUpdateEnd();
        timestampsPref.valueUpdateEnd();

        return true;
    }

private:
    AsyncPreference<std::array<Client, MaxRecords>> clientsPref;
    AsyncPreference<std::array<uint64_t, MaxRecords>> timestampsPref;

    auto idxById(const BleAuthId& client_id) -> int8_t {
        if (isIdProhibited(client_id)) { return -1; }

        auto& clients = clientsPref.get();

        auto it = std::find_if(clients.begin(), clients.end(), [&client_id](const Client& client) {
            return client.id == client_id;
        });
        if (it == clients.end()) { return -1; }

        return static_cast<int8_t>(std::distance(clients.begin(), it));
    }

    auto idxLRU() -> int8_t {
        auto& timestamps = timestampsPref.get();
        auto min_it = std::min_element(timestamps.begin(), timestamps.end());
        return static_cast<int8_t>(std::distance(timestamps.begin(), min_it));
    }

    auto isIdProhibited(const BleAuthId& client_id) const -> bool {
        return std::all_of(client_id.begin(), client_id.end(), [](auto val) {
            return val == 0;
        });
    }
};
