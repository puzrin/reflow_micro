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

    std::vector<uint8_t> buffer(DeviceStatus_size);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());

    pb_encode(&stream, DeviceStatus_fields, &status);
    buffer.resize(stream.bytes_written);

    return buffer;
}

std::vector<uint8_t> get_history_chunk(int32_t client_history_version, int32_t from) {
    std::vector<uint8_t> pb_data(HistoryChunk_size);

    app.heater.get_history(client_history_version, from, pb_data);
    return pb_data;
}

std::vector<uint8_t> get_profiles_data() {
    std::vector<uint8_t> pb_data(ProfilesData_size);
    app.profilesConfig.get_profiles(pb_data);
    return pb_data;
}

bool save_profiles_data(std::vector<uint8_t> pb_data) {
    app.profilesConfig.set_profiles(pb_data);
    return true;
}

bool stop() {
    app.safe_receive(AppCmd::Stop());
    return app.get_state_id() == DeviceState_Idle;
}

bool run_reflow() {
    app.safe_receive(AppCmd::Reflow());
    return app.get_state_id() == DeviceState_Reflow;
}

bool run_sensor_bake(float watts) {
    app.safe_receive(AppCmd::SensorBake(watts));
    return app.get_state_id() == DeviceState_SensorBake;
}

bool run_adrc_test(float temperature) {
    app.safe_receive(AppCmd::AdrcTest(temperature));
    return app.get_state_id() == DeviceState_AdrcTest;
}

bool run_step_response(float watts) {
    app.safe_receive(AppCmd::StepResponse(watts));
    return app.get_state_id() == DeviceState_StepResponse;
}

bool set_sensor_calibration_point(uint32_t point_id, float temperature) {
    if (!app.heater.is_hotplate_connected() ||
        !app.heater.set_sensor_calibration_point(point_id, temperature))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
}

std::vector<uint8_t> get_sensor_params() {
    std::vector<uint8_t> pb_data(SensorParams_size);
    if (!app.heater.is_hotplate_connected() ||
        !app.heater.get_sensor_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return pb_data;
}

bool set_adrc_params(std::vector<uint8_t> pb_data) {
    if (!app.heater.is_hotplate_connected() ||
        !app.heater.set_adrc_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
}

std::vector<uint8_t> get_adrc_params() {
    std::vector<uint8_t> pb_data(AdrcParams_size);
    if (!app.heater.is_hotplate_connected() ||
        !app.heater.get_adrc_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }

    return pb_data;
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
