#include <pb_encode.h>

#include <etl/string.h>
#include <etl/vector.h>

#include "api.hpp"
#include "app.hpp"
#include "components/pb2struct.hpp"
#include "components/profiles_config.hpp"
#include "heater/heater.hpp"
#include "rpc.hpp"
#include "proto/generated/shared_constants.hpp"
#include "proto/generated/types.pb.h"

namespace {

using RpcParams = cbor_rpc_dispatcher::ParamsReader;
using RpcResponse = RpcDispatcher::Response;

constexpr size_t RPC_ENVELOPE_SLACK = 64;
static_assert(ProfilesData_size + RPC_ENVELOPE_SLACK <= SharedConstants::MAX_RPC_MESSAGE_SIZE,
    "ProfilesData RPC response exceeds MAX_RPC_MESSAGE_SIZE");
static_assert(HistoryChunk_size + RPC_ENVELOPE_SLACK <= SharedConstants::MAX_RPC_MESSAGE_SIZE,
    "HistoryChunk RPC response exceeds MAX_RPC_MESSAGE_SIZE");
static_assert(DeviceInfo_size + RPC_ENVELOPE_SLACK <= SharedConstants::MAX_RPC_MESSAGE_SIZE,
    "DeviceInfo RPC response exceeds MAX_RPC_MESSAGE_SIZE");
static_assert(HeadParams_size + RPC_ENVELOPE_SLACK <= SharedConstants::MAX_RPC_MESSAGE_SIZE,
    "HeadParams RPC response exceeds MAX_RPC_MESSAGE_SIZE");

template <typename T, size_t Size>
auto encode_pb(const T& message, const pb_msgdesc_t* fields, etl::vector<uint8_t, Size>& output, size_t max_size) -> bool {
    output.clear();
    output.resize(max_size);

    pb_ostream_t stream = pb_ostream_from_buffer(output.data(), output.size());
    if (!pb_encode(&stream, fields, &message)) {
        output.clear();
        return false;
    }

    output.resize(stream.bytes_written);
    return true;
}

void get_status(const RpcParams& params, RpcResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

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

    etl::vector<uint8_t, DeviceInfo_size> buffer{};
    if (!encode_pb(status, DeviceInfo_fields, buffer, DeviceInfo_size)) {
        response.write_error("Failed to encode status");
        return;
    }

    response.write_binary(buffer);
}

void get_history_chunk(const RpcParams& params, RpcResponse& response) {
    int32_t client_history_version = 0;
    float from = 0;
    if (!params.has_count(2) ||
        !params.get_int32(0, client_history_version) ||
        !params.get_float(1, from))
    {
        response.write_error("Invalid params");
        return;
    }

    etl::vector<uint8_t, HistoryChunk_size> pb_data{};
    heater.get_history(client_history_version, from, pb_data);
    response.write_binary(pb_data);
}

void get_profiles_data(const RpcParams& params, RpcResponse& response) {
    bool reset = false;
    if (!params.has_count(1) || !params.get_bool(0, reset)) {
        response.write_error("Invalid params");
        return;
    }

    if (reset) {
        profiles_config.reset_profiles();
    }

    etl::vector<uint8_t, ProfilesData_size> pb_data{};
    if (!profiles_config.get_profiles(pb_data)) {
        response.write_error("Failed to load profiles");
        return;
    }

    response.write_binary(pb_data);
}

void save_profiles_data(const RpcParams& params, RpcResponse& response) {
    etl::vector<uint8_t, ProfilesData_size> pb_data{};
    if (!params.has_count(1) || !params.get_binary(0, pb_data)) {
        response.write_error("Invalid params");
        return;
    }

    if (!profiles_config.set_profiles(pb_data)) {
        response.write_error("Failed to save profiles");
        return;
    }

    response.write_bool(true);
}

void stop(const RpcParams& params, RpcResponse& response) {
    bool succeeded = false;
    if (!params.has_count(1) || !params.get_bool(0, succeeded)) {
        response.write_error("Invalid params");
        return;
    }

    application.receive(AppCmd::Stop{succeeded});
    response.write_bool(application.get_state_id() == DeviceActivityStatus_IDLE);
}

void run_reflow(const RpcParams& params, RpcResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

    application.receive(AppCmd::Reflow{});
    response.write_bool(application.get_state_id() == DeviceActivityStatus_REFLOW);
}

void run_sensor_bake(const RpcParams& params, RpcResponse& response) {
    float watts = 0;
    if (!params.has_count(1) || !params.get_float(0, watts)) {
        response.write_error("Invalid params");
        return;
    }

    application.receive(AppCmd::SensorBake{watts});
    response.write_bool(application.get_state_id() == DeviceActivityStatus_SENSOR_BAKE);
}

void run_adrc_test(const RpcParams& params, RpcResponse& response) {
    float temperature = 0;
    if (!params.has_count(1) || !params.get_float(0, temperature)) {
        response.write_error("Invalid params");
        return;
    }

    application.receive(AppCmd::AdrcTest{temperature});
    response.write_bool(application.get_state_id() == DeviceActivityStatus_ADRC_TEST);
}

void run_step_response(const RpcParams& params, RpcResponse& response) {
    float watts = 0;
    if (!params.has_count(1) || !params.get_float(0, watts)) {
        response.write_error("Invalid params");
        return;
    }

    application.receive(AppCmd::StepResponse{watts});
    response.write_bool(application.get_state_id() == DeviceActivityStatus_STEP_RESPONSE);
}

void get_head_params(const RpcParams& params, RpcResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

    etl::vector<uint8_t, HeadParams_size> pb_data{};
    if (heater.get_head_status() != HeadStatus_HEAD_CONNECTED || !heater.get_head_params_pb(pb_data)) {
        response.write_error("Hotplate is not connected");
        return;
    }

    response.write_binary(pb_data);
}

void set_head_params(const RpcParams& params, RpcResponse& response) {
    etl::vector<uint8_t, HeadParams_size> pb_data{};
    if (!params.has_count(1) || !params.get_binary(0, pb_data)) {
        response.write_error("Invalid params");
        return;
    }

    if (heater.get_head_status() != HeadStatus_HEAD_CONNECTED || !heater.set_head_params_pb(pb_data)) {
        response.write_error("Hotplate is not connected");
        return;
    }

    response.write_bool(true);
}

void set_cpoint0(const RpcParams& params, RpcResponse& response) {
    float temperature = 0;
    if (!params.has_count(1) || !params.get_float(0, temperature)) {
        response.write_error("Invalid params");
        return;
    }

    if (heater.get_head_status() != HeadStatus_HEAD_CONNECTED) {
        response.write_error("Hotplate is not connected");
        return;
    }
    if (!heater.set_calibration_point_0(temperature)) {
        response.write_error("Failed to set calibration point 0");
        return;
    }

    response.write_bool(true);
}

void set_cpoint1(const RpcParams& params, RpcResponse& response) {
    float temperature = 0;
    if (!params.has_count(1) || !params.get_float(0, temperature)) {
        response.write_error("Invalid params");
        return;
    }

    if (heater.get_head_status() != HeadStatus_HEAD_CONNECTED) {
        response.write_error("Hotplate is not connected");
        return;
    }
    if (!heater.set_calibration_point_1(temperature)) {
        response.write_error("Failed to set calibration point 1");
        return;
    }

    response.write_bool(true);
}

void get_pd_profiles(const RpcParams& params, RpcResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

    etl::vector<uint8_t, pd::MaxPdoObjects * sizeof(uint32_t)> raw_pdos{};
    raw_pdos.reserve(power.source_caps.size() * sizeof(uint32_t));

    for (auto pdo : power.source_caps) {
        raw_pdos.push_back(static_cast<uint8_t>(pdo & 0xFF));
        raw_pdos.push_back(static_cast<uint8_t>((pdo >> 8) & 0xFF));
        raw_pdos.push_back(static_cast<uint8_t>((pdo >> 16) & 0xFF));
        raw_pdos.push_back(static_cast<uint8_t>((pdo >> 24) & 0xFF));
    }

    response.write_binary(raw_pdos);
}

void get_ble_name(const RpcParams& params, RpcResponse& response) {
    if (!params.has_count(0)) {
        response.write_error("Invalid params");
        return;
    }

    auto name = ble_name_read();
    response.write_string(name.data(), name.size());
}

void set_ble_name(const RpcParams& params, RpcResponse& response) {
    BleName name{};
    if (!params.has_count(1) || !params.get_text(0, name)) {
        response.write_error("Invalid params");
        return;
    }

    if (name.empty()) {
        response.write_error("BLE name must not be empty");
        return;
    }

    ble_name_write(name);
    response.write_bool(true);
}

} // namespace

void api_methods_create() {
    rpc.addMethod("get_status", RpcDispatcher::MethodHandler::create<get_status>());
    rpc.addMethod("get_history_chunk", RpcDispatcher::MethodHandler::create<get_history_chunk>());
    rpc.addMethod("get_profiles_data", RpcDispatcher::MethodHandler::create<get_profiles_data>());
    rpc.addMethod("save_profiles_data", RpcDispatcher::MethodHandler::create<save_profiles_data>());
    rpc.addMethod("stop", RpcDispatcher::MethodHandler::create<stop>());
    rpc.addMethod("run_reflow", RpcDispatcher::MethodHandler::create<run_reflow>());
    rpc.addMethod("run_sensor_bake", RpcDispatcher::MethodHandler::create<run_sensor_bake>());
    rpc.addMethod("run_adrc_test", RpcDispatcher::MethodHandler::create<run_adrc_test>());
    rpc.addMethod("run_step_response", RpcDispatcher::MethodHandler::create<run_step_response>());
    rpc.addMethod("get_head_params", RpcDispatcher::MethodHandler::create<get_head_params>());
    rpc.addMethod("set_head_params", RpcDispatcher::MethodHandler::create<set_head_params>());
    rpc.addMethod("set_cpoint0", RpcDispatcher::MethodHandler::create<set_cpoint0>());
    rpc.addMethod("set_cpoint1", RpcDispatcher::MethodHandler::create<set_cpoint1>());
    rpc.addMethod("get_pd_profiles", RpcDispatcher::MethodHandler::create<get_pd_profiles>());
    rpc.addMethod("get_ble_name", RpcDispatcher::MethodHandler::create<get_ble_name>());
    rpc.addMethod("set_ble_name", RpcDispatcher::MethodHandler::create<set_ble_name>());
}
