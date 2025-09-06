#include "idle.hpp"
#include "logger.hpp"

auto Idle_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => Idle");
    return No_State_Change;
}

auto Idle_State::on_event(const AppCmd::Reflow&) -> etl::fsm_state_id_t {
    return DeviceState_Reflow;
}

auto Idle_State::on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.watts;
    return DeviceState_SensorBake;
}

auto Idle_State::on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.temperature;
    return DeviceState_AdrcTest;
}

auto Idle_State::on_event(const AppCmd::StepResponse& event) -> etl::fsm_state_id_t {
    get_fsm_context().last_cmd_data = event.watts;
    return DeviceState_StepResponse;
}

auto Idle_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    switch (event.type) {
        case ButtonEventId::BUTTON_PRESSED_5X:
            return DeviceState_Bonding;

        // Animate long press start
        case ButtonEventId::BUTTON_LONG_PRESS_START:
            APP_LOGI("Long press start");
            get_fsm_context().showLongPressProgress();
            break;

        // Stops animation if long press not reached
        case ButtonEventId::BUTTON_LONG_PRESS_FAIL:
            APP_LOGI("Long press fail");
            get_fsm_context().blinker.off();
            break;

        case ButtonEventId::BUTTON_LONG_PRESS:
            APP_LOGI("Long press succeeded");
            return DeviceState_Reflow;

        default:
            break;
    }
    return No_State_Change;
}

auto Idle_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}
