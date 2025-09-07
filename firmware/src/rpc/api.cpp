#include <vector>
#include <pb_encode.h>

#include "api.hpp"
#include "components/profiles_config.hpp"
#include "heater/heater.hpp"
#include "rpc.hpp"
#include "proto/generated/types.pb.h"
#include "app.hpp"

namespace {

std::vector<uint8_t> get_status() {
    DeviceStatus status = {
        .state = static_cast<DeviceState>(application.get_state_id()),
        .hotplate_connected = heater.is_hotplate_connected(),
        .hotplate_id = heater.get_hotplate_id(),
        .temperature = heater.get_temperature(),
        .watts = heater.get_power(),
        .volts = heater.get_volts(),
        .amperes = heater.get_amperes(),
        .max_watts = heater.get_max_power(),
        .duty_cycle = heater.get_duty_cycle(),
        .resistance = heater.get_resistance()
    };

    std::vector<uint8_t> buffer(DeviceStatus_size);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());

    pb_encode(&stream, DeviceStatus_fields, &status);
    buffer.resize(stream.bytes_written);

    return buffer;
}

std::vector<uint8_t> get_history_chunk(int32_t client_history_version, float from) {
    std::vector<uint8_t> pb_data(HistoryChunk_size);

    heater.get_history(client_history_version, from, pb_data);
    return pb_data;
}

std::vector<uint8_t> get_profiles_data(bool reset) {
    if (reset) { profiles_config.reset_profiles(); }

    std::vector<uint8_t> pb_data(ProfilesData_size);
    profiles_config.get_profiles(pb_data);
    return pb_data;
}

auto save_profiles_data(std::vector<uint8_t> pb_data) -> bool {
    profiles_config.set_profiles(pb_data);
    return true;
}

auto stop() -> bool {
    application.receive(AppCmd::Stop{});
    return application.get_state_id() == DeviceState_Idle;
}

auto run_reflow() -> bool {
    application.receive(AppCmd::Reflow{});
    return application.get_state_id() == DeviceState_Reflow;
}

auto run_sensor_bake(float watts) -> bool {
    application.receive(AppCmd::SensorBake{watts});
    return application.get_state_id() == DeviceState_SensorBake;
}

auto run_adrc_test(float temperature) -> bool {
    application.receive(AppCmd::AdrcTest{temperature});
    return application.get_state_id() == DeviceState_AdrcTest;
}

auto run_step_response(float watts) -> bool {
    application.receive(AppCmd::StepResponse{watts});
    return application.get_state_id() == DeviceState_StepResponse;
}

std::vector<uint8_t> get_head_params() {
    std::vector<uint8_t> pb_data(HeadParams_size);
    if (!heater.is_hotplate_connected() ||
        !heater.get_head_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }

    return pb_data;
}

auto set_head_params(std::vector<uint8_t> pb_data) -> bool {
    if (!heater.is_hotplate_connected() ||
        !heater.set_head_params(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
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
    rpc.addMethod("get_head_params", get_head_params);
    rpc.addMethod("set_head_params", set_head_params);
}
