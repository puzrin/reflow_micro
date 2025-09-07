#include "sensor_bake.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"

auto SensorBake_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => SensorBake");

    auto& app = get_fsm_context();

    if (!heater.task_start(HISTORY_ID_SENSOR_BAKE_MODE)) { return DeviceState_Idle; }

    heater.set_power(app.last_cmd_data);

    return No_State_Change;
}

auto SensorBake_State::on_event(const AppCmd::Stop&) -> etl::fsm_state_id_t {
    return DeviceState_Idle;
}

auto SensorBake_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) { return DeviceState_Idle; }
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
