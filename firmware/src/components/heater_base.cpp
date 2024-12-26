#include "heater_base.hpp"
#include <pb_encode.h>
#include <pb_decode.h>
#include <cmath>
#include <algorithm>

std::vector<uint8_t> HeaterBase::get_adrc_params_pb() {
    if (is_hotplate_connected()) return adrc_params[get_hotplate_id()].get();

    // Return default to avoid error/exception
    return std::vector<uint8_t>(std::vector<uint8_t>{std::begin(DEFAULT_ADRC_PARAMS_PB), std::end(DEFAULT_ADRC_PARAMS_PB)});
}

bool HeaterBase::set_adrc_params_pb(const std::vector<uint8_t> &pb_data) {
    if (!is_hotplate_connected()) return false;

    adrc_params[get_hotplate_id()].set(pb_data);
    load_all_params(); // Auto-reload all heater params on update
    return true;
}

AdrcParams HeaterBase::get_adrc_params() {
    auto pb_data = get_adrc_params_pb();
    AdrcParams params;

    pb_istream_t stream = pb_istream_from_buffer(pb_data.data(), pb_data.size());
    if (!pb_decode(&stream, AdrcParams_fields, &params)) return AdrcParams_init_default;

    return params;
}

bool HeaterBase::set_adrc_params(const AdrcParams& params) {
    uint8_t buffer[AdrcParams_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    if (!pb_encode(&stream, AdrcParams_fields, &params)) return false;

    return set_adrc_params_pb(std::vector<uint8_t>(buffer, buffer + stream.bytes_written));
}

std::vector<uint8_t> HeaterBase::get_sensor_params_pb() {
    if (is_hotplate_connected()) return sensor_params[get_hotplate_id()].get();

    // Return default to avoid error/exception
    return std::vector<uint8_t>(std::vector<uint8_t>{std::begin(DEFAULT_SENSOR_PARAMS_PB), std::end(DEFAULT_SENSOR_PARAMS_PB)});
}

bool HeaterBase::set_sensor_params_pb(const std::vector<uint8_t>& pb_data) {
    if (!is_hotplate_connected()) return false;

    sensor_params[get_hotplate_id()].set(pb_data);
    load_all_params(); // Auto-reload all heater params on update
    return true;
}

SensorParams HeaterBase::get_sensor_params() {
   auto pb_data = get_sensor_params_pb();
   SensorParams params;

   pb_istream_t stream = pb_istream_from_buffer(pb_data.data(), pb_data.size());
   if (!pb_decode(&stream, SensorParams_fields, &params)) return SensorParams_init_default;

   return params;
}

bool HeaterBase::set_sensor_params(const SensorParams& params) {
   uint8_t buffer[SensorParams_size];
   pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

   if (!pb_encode(&stream, SensorParams_fields, &params)) return false;

   return set_sensor_params_pb(std::vector<uint8_t>(buffer, buffer + stream.bytes_written));
}

void HeaterBase::get_history_pb(uint32_t client_history_version, int32_t from, std::vector<uint8_t>& pb_data) {
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
                for (int i = data.size() - 1; i >= 0; --i) {
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


void HeaterBase::load_all_params() {
    auto p = get_adrc_params();
    adrc.set_params(p.b0, p.response, p.N, p.M);
}

void HeaterBase::temperature_control_on() {
    load_all_params();
    adrc.reset_to(get_temperature());
    temperature_control_flag = true;
}

void HeaterBase::temperature_control_off() {
    temperature_control_flag = false;
    set_power(0);
}

void HeaterBase::iterate(uint32_t dt_ms) {
    if (!temperature_control_flag) return;

    static constexpr float dt_inv_multiplier = 1.0f / 1000.0f;
    float dt = dt_ms * dt_inv_multiplier;

    float power = adrc.iterate(get_temperature(), temperature_setpoint, get_max_power(), dt);
    set_power(power);

    if (is_task_active) {
        task_time_ms += dt_ms;
        task_tick_common(dt_ms);
        if (task_ticker) task_ticker(get_temperature(), task_time_ms);
    }
}

void HeaterBase::task_start(int32_t task_id, HeaterTaskTickerFn ticker) {
    history.data.clear();
    history.set_params(2, history_y_multiplier * 1, 400);
    task_time_ms = 0;
    history_last_recorded_ts = 0;
    history_task_id = task_id;
    history_version++;

    // Add first point
    history.add(0, lround(get_temperature() * history_y_multiplier));

    task_ticker = ticker;
    is_task_active = true;
}

void HeaterBase::task_stop() {
    is_task_active = false;
    task_ticker = nullptr;
    temperature_control_off();
    set_power(0);
};


void HeaterBase::task_tick_common(uint32_t dt_ms) {
    const uint32_t seconds = task_time_ms / 1000;

    if (seconds > history_last_recorded_ts) {
        history.add(seconds, lround(get_temperature() * history_y_multiplier));
        history_last_recorded_ts = seconds;
    }
}
