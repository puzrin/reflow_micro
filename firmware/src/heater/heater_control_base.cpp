#include <cmath>
#include <algorithm>
#include <memory>

#include "heater_control_base.hpp"
#include "components/pb2struct.hpp"

void HeaterControlBase::get_history(int32_t client_history_version, float from, std::vector<uint8_t>& pb_data) {
    size_t from_idx{0};
    size_t chunk_length{0};
    auto history_chunk = std::make_unique<HistoryChunk>();
    auto& data = history.data;
    int32_t int_from = lround(from);

    history.lock();

    if (history_version != client_history_version) {
        // If client version mismatch - send from the beginning
        from_idx = 0;
        chunk_length = std::min(data.size(), static_cast<size_t>(MAX_HISTORY_CHUNK));
    } else {
        if (data.empty() || data.back().x < int_from) {
            // If no data - send empty
            from_idx = 0;
            chunk_length = 0;
        } else {
            // Search from the back, that's usually faster
            if (data.front().x >= int_from) {
                // Special case, nothing to skip
                from_idx = 0;
            } else {
                for (int32_t i = data.size() - 1; i >= 0; --i) {
                    if (data[i].x < int_from) {
                        from_idx = i + 1;
                        break;
                    }
                }
            }
            chunk_length = std::min(data.size() - from_idx, static_cast<size_t>(MAX_HISTORY_CHUNK));
        }
    }

    // Fill protobuf struct
    history_chunk->type = history_task_id;
    history_chunk->version = history_version;
    history_chunk->data_count = chunk_length;

    for (size_t i = 0; i < chunk_length; ++i) {
        history_chunk->data[i].x = static_cast<float>(data[from_idx + i].x);
        history_chunk->data[i].y = static_cast<float>(data[from_idx + i].y) * history_y_multiplier_inv;
    }

    history.unlock();

    struct2pb(*history_chunk, pb_data, HistoryChunk_fields, HistoryChunk_size);
}


auto HeaterControlBase::load_all_params() -> bool {
    HeadParams p;
    if (!get_head_params(p)) { return false; }
    adrc.set_params(p.adrc_b0, p.adrc_response, p.adrc_N, p.adrc_M);
    return true;
}

void HeaterControlBase::temperature_control_on() {
    load_all_params();

    adrc.reset_to(get_temperature());
    temperature_control_enabled = true;
}

void HeaterControlBase::temperature_control_off() {
    temperature_control_enabled = false;
    set_power(0);
}

void HeaterControlBase::tick() {
    uint32_t now = get_time_ms();
    uint32_t dt_ms = now - prev_tick_ms;

    // If temperature controller active - use it to update power
    if (is_task_active.load()) {
        if (temperature_control_enabled) {
            static constexpr float dt_inv_multiplier = 1.0F / 1000;
            const float dt = static_cast<float>(dt_ms) * dt_inv_multiplier;

            const float power = adrc.iterate(get_temperature(), temperature_setpoint, get_max_power(), dt);
            set_power(power);
        }

        // Write history every second
        uint32_t task_time_ms = now - task_start_ts;

        const uint32_t seconds = task_time_ms / 1000;
        if (seconds > history_last_recorded_ts) {
            history.add(seconds, lround(get_temperature() * history_y_multiplier));
            history_last_recorded_ts = seconds;
        }

        // Task can have custom iterator, execute it is needed
        if (task_iterator) task_iterator(task_time_ms);
    }

    prev_tick_ms = now;
}

auto HeaterControlBase::task_start(int32_t task_id, HeaterTaskIteratorFn ticker) -> bool {
    if (is_task_active.load()) { return false; }
    if (get_head_status() != HeadStatus_HeadConnected) { return false; }
    if (!load_all_params()) { return false; }

    history.data.clear();
    history.set_params(2, history_y_multiplier * 1, 400);
    task_start_ts = get_time_ms();
    history_last_recorded_ts = 0;
    history_task_id = task_id;
    history_version++;

    // Add first point
    history.add(0, lround(get_temperature() * history_y_multiplier));

    task_iterator = ticker;
    is_task_active.store(true);
    return true;
}

void HeaterControlBase::task_stop() {
    is_task_active.store(false);
    task_iterator = nullptr;
    temperature_control_off();
    set_power(0);
};
