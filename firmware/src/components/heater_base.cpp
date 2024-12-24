#include "heater_base.hpp"
#include <pb_encode.h>
#include <pb_decode.h>

std::vector<uint8_t> HeaterBase::get_adrc_params_pb() {
    if (is_hotplate_connected()) return adrc_params[get_hotplate_id()];

    // Return default to avoid error/exception
    return std::vector<uint8_t>(std::vector<uint8_t>{std::begin(DEFAULT_ADRC_PARAMS_PB), std::end(DEFAULT_ADRC_PARAMS_PB)});
}

bool HeaterBase::set_adrc_params_pb(const std::vector<uint8_t> &pb_data) {
    if (!is_hotplate_connected()) return false;

    adrc_params[get_hotplate_id()] = pb_data;
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
    if (is_hotplate_connected()) return sensor_params[get_hotplate_id()];

    // Return default to avoid error/exception
    return std::vector<uint8_t>(std::vector<uint8_t>{std::begin(DEFAULT_SENSOR_PARAMS_PB), std::end(DEFAULT_SENSOR_PARAMS_PB)});
}

bool HeaterBase::set_sensor_params_pb(const std::vector<uint8_t>& pb_data) {
    if (!is_hotplate_connected()) return false;

    sensor_params[get_hotplate_id()] = pb_data;
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

void HeaterBase::load_all_params() {
    auto p = get_adrc_params();
    adrc.set_params(p.b0, p.response, p.N, p.M);
}

void HeaterBase::temperature_control_on() {
    adrc.reset_to(get_temperature());
    temperature_control_flag = true;
}

void HeaterBase::temperature_control_off() {
    temperature_control_flag = false;
    set_power(0);
}

void HeaterBase::iterate(float dt) {
    if (!temperature_control_flag) return;

    float power = adrc.iterate(get_temperature(), temperature_setpoint, get_max_power(), dt);
    set_power(power);
}