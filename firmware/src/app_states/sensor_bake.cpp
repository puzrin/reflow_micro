#include "sensor_bake.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"

auto SensorBake_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => SensorBake");

    auto& app = get_fsm_context();

    last_temperature = heater.get_temperature();

    auto status = heater.task_start(HISTORY_ID_SENSOR_BAKE_MODE, [this](int32_t dt_ms, int32_t time_ms) {
        task_iterator(dt_ms, time_ms);
    });
    if (!status) { return DeviceActivityStatus_Idle; }

    heater.set_power(app.last_cmd_data);

    return No_State_Change;
}

auto SensorBake_State::on_event(const AppCmd::Stop&) -> etl::fsm_state_id_t {
    return DeviceActivityStatus_Idle;
}

auto SensorBake_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) { return DeviceActivityStatus_Idle; }
    return No_State_Change;
}

auto SensorBake_State::on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t {
    heater.set_power(event.watts);
    return No_State_Change;
}

auto SensorBake_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void SensorBake_State::on_exit_state() {
    heater.task_stop();
}

void SensorBake_State::task_iterator(int32_t /*dt_ms*/, int32_t /*time_ms*/) {
    // Check for abnormal temperature jitter
    const float current_temp = heater.get_temperature();
    if (std::abs(current_temp - last_temperature) > 5.0f) {
        APP_LOGE("Abnormal temperature jitter detected: {} -> {}",
            static_cast<int>(last_temperature), static_cast<int>(current_temp));
    }
    last_temperature = current_temp;
}
