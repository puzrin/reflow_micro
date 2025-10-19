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
    DeviceInfo status = {
        .health = heater.get_health_status(),
        .activity = heater.get_activity_status(),
        .power = heater.get_power_status(),
        .head = heater.get_head_status(),
        .temperature_x10 = static_cast<int32_t>(heater.get_temperature() * 10),
        .peak_mv = static_cast<uint32_t>(heater.get_volts() * 1000),
        .peak_ma = static_cast<uint32_t>(heater.get_amperes() * 1000),
        .duty_x1000 = static_cast<uint32_t>(heater.get_duty_cycle() * 1000),
        .resistance_mohms = static_cast<uint32_t>(heater.get_resistance() * 1000),
        .max_mw = static_cast<uint32_t>(heater.get_max_power() * 1000)
    };

    std::vector<uint8_t> buffer(DeviceInfo_size);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());

    pb_encode(&stream, DeviceInfo_fields, &status);
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

auto stop(bool succeeded) -> bool {
    application.receive(AppCmd::Stop{succeeded});
    return application.get_state_id() == DeviceActivityStatus_Idle;
}

auto run_reflow() -> bool {
    application.receive(AppCmd::Reflow{});
    return application.get_state_id() == DeviceActivityStatus_Reflow;
}

auto run_sensor_bake(float watts) -> bool {
    application.receive(AppCmd::SensorBake{watts});
    return application.get_state_id() == DeviceActivityStatus_SensorBake;
}

auto run_adrc_test(float temperature) -> bool {
    application.receive(AppCmd::AdrcTest{temperature});
    return application.get_state_id() == DeviceActivityStatus_AdrcTest;
}

auto run_step_response(float watts) -> bool {
    application.receive(AppCmd::StepResponse{watts});
    return application.get_state_id() == DeviceActivityStatus_StepResponse;
}

std::vector<uint8_t> get_head_params() {
    std::vector<uint8_t> pb_data(HeadParams_size);
    if (heater.get_head_status() != HeadStatus_HeadConnected ||
        !heater.get_head_params_pb(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }

    return pb_data;
}

auto set_head_params(std::vector<uint8_t> pb_data) -> bool {
    if (heater.get_head_status() != HeadStatus_HeadConnected ||
        !heater.set_head_params_pb(pb_data))
    {
        throw std::runtime_error("Hotplate is not connected");
    }
    return true;
}

auto set_cpoint0(float temperature) -> bool {
    if (heater.get_head_status() != HeadStatus_HeadConnected) {
        throw std::runtime_error("Hotplate is not connected");
    }
    if (!heater.set_calibration_point_0(temperature)) {
        throw std::runtime_error("Failed to set calibration point 0");
    }
    return true;
}

auto set_cpoint1(float temperature) -> bool {
    if (heater.get_head_status() != HeadStatus_HeadConnected) {
        throw std::runtime_error("Hotplate is not connected");
    }
    if (!heater.set_calibration_point_1(temperature)) {
        throw std::runtime_error("Failed to set calibration point 1");
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
    rpc.addMethod("set_cpoint0", set_cpoint0);
    rpc.addMethod("set_cpoint1", set_cpoint1);
}
