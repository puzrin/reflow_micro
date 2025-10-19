#include "idle.hpp"
#include "logger.hpp"

auto Idle_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => Idle");
    return No_State_Change;
}

auto Idle_State::on_event(const AppCmd::Reflow&) -> etl::fsm_state_id_t {
    return DeviceActivityStatus_Reflow;
}

auto Idle_State::on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.watts;
    return DeviceActivityStatus_SensorBake;
}

auto Idle_State::on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.temperature;
    return DeviceActivityStatus_AdrcTest;
}

auto Idle_State::on_event(const AppCmd::StepResponse& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.watts;
    return DeviceActivityStatus_StepResponse;
}

auto Idle_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    switch (event.type) {
        case ButtonEventId::BUTTON_PRESSED_5X:
            return DeviceActivityStatus_Bonding;

        // Animate long press start
        case ButtonEventId::BUTTON_LONG_PRESS_START:
            APP_LOGI("Long press start");
            app.showLongPressProgress();
            break;

        // Stops animation if long press not reached
        case ButtonEventId::BUTTON_LONG_PRESS_FAIL:
            APP_LOGI("Long press fail");
            app.showOff();
            break;

        case ButtonEventId::BUTTON_LONG_PRESS:
            APP_LOGI("Long press succeeded");
            app.showOff();
            return DeviceActivityStatus_Reflow;

        default:
            break;
    }
    return No_State_Change;
}

auto Idle_State::on_event(const AppCmd::Stop&) -> etl::fsm_state_id_t {
    // This empty handler exists only to dim unknown event reporting
    return No_State_Change;
}

auto Idle_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void Idle_State::on_exit_state() {
    auto& app = get_fsm_context();

    // Ensure to disable test animations
    app.showOff();
}
