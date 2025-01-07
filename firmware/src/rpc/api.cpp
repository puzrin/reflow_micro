#include <vector>
#include "api.hpp"
#include "rpc.hpp"
#include "proto/generated/types.pb.h"
#include "app.hpp"
#include <pb_encode.h>

namespace {

std::vector<uint8_t> get_status() {
    DeviceStatus status = {
        .state = static_cast<DeviceState>(application.get_state_id()),
        .hotplate_connected = application.heater.is_hotplate_connected(),
        .hotplate_id = application.heater.get_hotplate_id(),
        .temperature = application.heater.get_temperature(),
        .watts = application.heater.get_power(),
        .volts = application.heater.get_volts(),
        .amperes = application.heater.get_amperes(),
        .max_watts = application.heater.get_max_power(),
        .duty_cycle = application.heater.get_duty_cycle(),
        .resistance = application.heater.get_resistance()
    };

    std::vector<uint8_t> buffer(DeviceStatus_size);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());

    pb_encode(&stream, DeviceStatus_fields, &status);
    buffer.resize(stream.bytes_written);

    return buffer;
}

std::vector<uint8_t> get_history_chunk(int32_t client_history_version, float from) {
    std::vector<uint8_t> pb_data(HistoryChunk_size);

    application.heater.get_history(client_history_version, from, pb_data);
    return pb_data;
}

std::vector<uint8_t> get_profiles_data(bool reset) {
    if (reset) { application.profilesConfig.reset_profiles(); }

    std::vector<uint8_t> pb_data(ProfilesData_size);
    application.profilesConfig.get_profiles(pb_data);
    return pb_data;
}

auto save_profiles_data(std::vector<uint8_t> pb_data) -> bool {
    application.profilesConfig.set_profiles(pb_data);
    return true;
}

auto stop() -> bool {
    application.safe_receive(AppCmd::Stop());
    return application.get_state_id() == DeviceState_Idle;
}

auto run_reflow() -> bool {
    application.safe_receive(AppCmd::Reflow());
    return application.get_state_id() == DeviceState_Reflow;
}

auto run_sensor_bake(float watts) -> bool {
    application.safe_receive(AppCmd::SensorBake(watts));
    return application.get_state_id() == DeviceState_SensorBake;
}

auto run_adrc_test(float temperature) -> bool {
    application.safe_receive(AppCmd::AdrcTest(temperature));
    return application.get_state_id() == DeviceState_AdrcTest;
}

auto run_step_response(float watts) -> bool {
    application.safe_receive(AppCmd::StepResponse(watts));
    return application.get_state_id() == DeviceState_StepResponse;
}

auto set_sensor_calibration_point(uint32_t point_id, float temperature) -> bool {
    if (!application.heater.is_hotplate_connected() ||
        !application.heater.set_sensor_calibration_point(point_id, temperature))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
}

std::vector<uint8_t> get_sensor_params() {
    std::vector<uint8_t> pb_data(SensorParams_size);
    if (!application.heater.is_hotplate_connected() ||
        !application.heater.get_sensor_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return pb_data;
}

auto set_adrc_params(std::vector<uint8_t> pb_data) -> bool {
    if (!application.heater.is_hotplate_connected() ||
        !application.heater.set_adrc_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
}

std::vector<uint8_t> get_adrc_params() {
    std::vector<uint8_t> pb_data(AdrcParams_size);
    if (!application.heater.is_hotplate_connected() ||
        !application.heater.get_adrc_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }

    return pb_data;
}

} // namespace

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
