#include <vector>
#include "api.hpp"
#include "rpc.hpp"
#include "proto/generated/types.pb.h"
#include "app.hpp"
#include <pb_encode.h>

namespace {

std::vector<uint8_t> get_status() {
    DeviceStatus status = {
        .state = static_cast<DeviceState>(app.get_state_id()),
        .hotplate_connected = app.heater.is_hotplate_connected(),
        .hotplate_id = app.heater.get_hotplate_id(),
        .temperature = app.heater.get_temperature(),
        .watts = app.heater.get_power(),
        .volts = app.heater.get_volts(),
        .amperes = app.heater.get_amperes(),
        .max_watts = app.heater.get_max_power(),
        .duty_cycle = app.heater.get_duty_cycle()
    };

    uint8_t buffer[DeviceStatus_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    pb_encode(&stream, DeviceStatus_fields, &status);

    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> get_history_chunk() {
    // TODO
    return {};
}

std::vector<uint8_t> get_profiles_data() {
    // TODO
    return {};
}

bool save_profiles_data(std::vector<uint8_t> pb_data) {
    // TODO
    return true;
}

bool stop() {
    // TODO
    return true;
}

bool run_reflow() {
    // TODO
    return true;
}

bool run_sensor_bake() {
    // TODO
    return true;
}

bool run_adrc_test() {
    // TODO
    return true;
}

bool run_step_response() {
    // TODO
    return true;
}

bool set_sensor_calibration_point(uint32_t point_id, float temperature) {
    if (!app.heater.is_hotplate_connected()) throw std::runtime_error("Hotplate is not connected");

    return app.heater.set_sensor_calibration_point(point_id, temperature);
}

std::vector<uint8_t> get_sensor_params() {
    return app.heater.get_sensor_params_pb();
}

bool set_adrc_params(std::vector<uint8_t> pb_data) {
    return app.heater.set_adrc_params_pb(pb_data);
}

std::vector<uint8_t> get_adrc_params() {
    return app.heater.get_adrc_params_pb();
}

}

void api_methods_create() {
    rpc.addMethod("get_status", get_status);
    rpc.addMethod("get_history_chunk", get_history_chunk);
    rpc.addMethod("get_profiles_data", get_profiles_data);
    rpc.addMethod("save_profiles_data", save_profiles_data);
    rpc.addMethod("stop", stop);
    rpc.addMethod("run_reflow", run_reflow);
    rpc.addMethod("run_sensor_bake", run_sensor_bake);
    rpc.addMethod("run_adrc_test", run_adrc_test);
    rpc.addMethod("run_step_response", run_step_response);
    rpc.addMethod("set_sensor_calibration_point", set_sensor_calibration_point);
    rpc.addMethod("get_sensor_params", get_sensor_params);
    rpc.addMethod("set_adrc_params", set_adrc_params);
    rpc.addMethod("get_adrc_params", get_adrc_params);
}
