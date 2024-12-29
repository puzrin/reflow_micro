#include "app.hpp"
#include "logger.hpp"
#include "blink_signals.hpp"

namespace {

class Idle : public etl::fsm_state<App, Idle, DeviceState_Idle,
    AppCmd::Reflow, AppCmd::SensorBake, AppCmd::AdrcTest, AppCmd::StepResponse, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() override {
        DEBUG("State => Idle");
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event(const AppCmd::Reflow& event) { return DeviceState_Reflow; }
    etl::fsm_state_id_t on_event(const AppCmd::SensorBake& event) {
        get_fsm_context().last_cmd_data = event.watts;
        return DeviceState_SensorBake;
    }
    etl::fsm_state_id_t on_event(const AppCmd::AdrcTest& event) {
        get_fsm_context().last_cmd_data = event.temperature;
        return DeviceState_AdrcTest;
    }
    etl::fsm_state_id_t on_event(const AppCmd::StepResponse& event) {
        get_fsm_context().last_cmd_data = event.watts;
        return DeviceState_StepResponse;
    }

    etl::fsm_state_id_t on_event(const AppCmd::Button& event) {
        switch (event.type) {
            case ButtonEventId::BUTTON_PRESSED_5X:
                return DeviceState_Bonding;

            // Animate long press start
            case ButtonEventId::BUTTON_LONG_PRESS_START:
                DEBUG("Long press start");
                BLINK_LONG_PRESS_START(get_fsm_context().blinker);
                break;

            // Stops animation if long press not reached
            case ButtonEventId::BUTTON_LONG_PRESS_FAIL:
                DEBUG("Long press fail");
                get_fsm_context().blinker.off();
                break;

            case ButtonEventId::BUTTON_LONG_PRESS:
                DEBUG("Long press succeeded");
                return DeviceState_Reflow;

            default:
                break;
        }
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

Idle idle;

} // namespace

etl::ifsm_state& state_idle = idle;
