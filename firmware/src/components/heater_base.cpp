#include "heater_base.hpp"
#include "components/pb2struct.hpp"
#include <cmath>
#include <algorithm>

bool HeaterBase::get_adrc_params(std::vector<uint8_t>& pb_data) {
    if (!is_hotplate_connected()) return false;

    pb_data = adrc_params[get_hotplate_id()].get();
    return true;
}

bool HeaterBase::get_adrc_params(AdrcParams& params) {
    if (!is_hotplate_connected()) return false;

    return pb2struct(adrc_params[get_hotplate_id()].get(), params, AdrcParams_fields);
}

bool HeaterBase::set_adrc_params(const std::vector<uint8_t> &pb_data) {
    if (!is_hotplate_connected()) return false;

    adrc_params[get_hotplate_id()].set(pb_data);
    return true;
}

bool HeaterBase::set_adrc_params(const AdrcParams& params) {
    if (!is_hotplate_connected()) return false;

    std::vector<uint8_t> pb_data(AdrcParams_size);
    if (!struct2pb(params, pb_data, AdrcParams_fields, AdrcParams_size)) return false;

    adrc_params[get_hotplate_id()].set(pb_data);
    return true;
}

bool HeaterBase::get_sensor_params(std::vector<uint8_t>& pb_data) {
    if (!is_hotplate_connected()) return false;

    pb_data = sensor_params[get_hotplate_id()].get();
    return true;
}

bool HeaterBase::get_sensor_params(SensorParams& params) {
    if (!is_hotplate_connected()) return false;

    return pb2struct(sensor_params[get_hotplate_id()].get(), params, SensorParams_fields);
}
bool HeaterBase::set_sensor_params(const std::vector<uint8_t>& pb_data) {
    if (!is_hotplate_connected()) return false;

    sensor_params[get_hotplate_id()].set(pb_data);
    return true;
}

bool HeaterBase::set_sensor_params(const SensorParams& params) {
    if (!is_hotplate_connected()) return false;

    std::vector<uint8_t> pb_data(SensorParams_size);
    if (!struct2pb(params, pb_data, SensorParams_fields, SensorParams_size)) return false;

    sensor_params[get_hotplate_id()].set(pb_data);
    return true;
}

void HeaterBase::get_history(int32_t client_history_version, int32_t from, std::vector<uint8_t>& pb_data) {
    size_t from_idx = 0;
    size_t chunk_length;
    auto history_chunk = new HistoryChunk();
    auto& data = history.data;

    history.lock();

    if (history_version == client_history_version) {
        // If client version mismatch - send from the beginning
        from_idx = 0;
        chunk_length = std::min(data.size(), static_cast<size_t>(MAX_HISTORY_CHUNK));
    } else {
        if (data.empty() || data.back().x < from) {
            // If no data - send empty
            from_idx = 0;
            chunk_length = 0;
        } else {
            // Search from the back, that's usually faster
            if (data.front().x >= from) {
                // Special case, nothing to skip
                from_idx = 0;
            } else {
                for (int32_t i = data.size() - 1; i >= 0; --i) {
                    if (data[i].x < from) {
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
        history_chunk->data[i].x = data[from_idx + i].x;
        history_chunk->data[i].y = data[from_idx + i].y;
    }

    history.unlock();

    pb_data.resize(HistoryChunk_size);
    pb_ostream_t stream = pb_ostream_from_buffer(pb_data.data(), pb_data.size());
    pb_encode(&stream, HistoryChunk_fields, history_chunk);
    pb_data.resize(stream.bytes_written);
}


bool HeaterBase::load_all_params() {
    AdrcParams p;
    if (!get_adrc_params(p)) return false;
    adrc.set_params(p.b0, p.response, p.N, p.M);
    return true;
}

void HeaterBase::temperature_control_on() {
    // If task was is running - it already loaded params, don't repeat
    if (!is_task_active) load_all_params();

    adrc.reset_to(get_temperature());
    temperature_control_flag = true;
}

void HeaterBase::temperature_control_off() {
    temperature_control_flag = false;
    set_power(0);
}

void HeaterBase::tick(int32_t dt_ms) {
    // If temperature controller active - use it to update power
    if (temperature_control_flag) {
        static constexpr float dt_inv_multiplier = 1.0f / 1000;
        float dt = dt_ms * dt_inv_multiplier;

        float power = adrc.iterate(get_temperature(), temperature_setpoint, get_max_power(), dt);
        set_power(power);
    }

    if (!is_task_active) return;
    task_time_ms += dt_ms;

    // Write history every second
    const uint32_t seconds = task_time_ms / 1000;
    if (seconds > history_last_recorded_ts) {
        history.add(seconds, lround(get_temperature() * history_y_multiplier));
        history_last_recorded_ts = seconds;
    }

    // Task can have custom iterator, execute it is needed
    if (task_iterator) task_iterator(dt_ms, task_time_ms);
}

bool HeaterBase::task_start(int32_t task_id, HeaterTaskIteratorFn ticker) {
    if (is_task_active) return false;
    if (!is_hotplate_connected()) return false;
    if (!load_all_params()) return false;

    history.data.clear();
    history.set_params(2, history_y_multiplier * 1, 400);
    task_time_ms = 0;
    history_last_recorded_ts = 0;
    history_task_id = task_id;
    history_version++;

    // Add first point
    history.add(0, lround(get_temperature() * history_y_multiplier));

    task_iterator = ticker;
    is_task_active = true;
    return true;
}

void HeaterBase::task_stop() {
    is_task_active = false;
    task_iterator = nullptr;
    temperature_control_off();
    set_power(0);
};
