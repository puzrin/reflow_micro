#include "adrc_test.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"


auto AdrcTest_State::on_enter_state() -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();
    APP_LOGI("State => AdrcTest");

    heater.set_temperature(app.last_cmd_data);
    if (!heater.task_start(HISTORY_ID_ADRC_TEST_MODE)) {
        app.beepTaskTerminated();
        return DeviceActivityStatus_Idle;
    }

    heater.temperature_control_on();
    app.beepTaskStarted();
    return No_State_Change;
}

auto AdrcTest_State::on_event(const AppCmd::Stop&) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    app.beepTaskSucceeded();
    return DeviceActivityStatus_Idle;
}
auto AdrcTest_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) {
        app.beepTaskSucceeded();
        return DeviceActivityStatus_Idle;
    }
    return No_State_Change;
}
auto AdrcTest_State::on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t {
    heater.set_temperature(event.temperature);
    return No_State_Change;
}

auto AdrcTest_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void AdrcTest_State::on_exit_state() {
    heater.task_stop();
}
