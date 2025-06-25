#include "app.hpp"
#include "logger.hpp"

namespace {

class Idle : public etl::fsm_state<App, Idle, DeviceState_Idle,
    AppCmd::Reflow, AppCmd::SensorBake, AppCmd::AdrcTest, AppCmd::StepResponse, AppCmd::Button> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override {
        DEBUG("State => Idle");
        return No_State_Change;
    }

    auto on_event(const AppCmd::Reflow& event) -> etl::fsm_state_id_t {
        (void)event;
        return DeviceState_Reflow; }
    auto on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t {
        get_fsm_context().last_cmd_data = event.watts;
        return DeviceState_SensorBake;
    }
    auto on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t {
        get_fsm_context().last_cmd_data = event.temperature;
        return DeviceState_AdrcTest;
    }
    auto on_event(const AppCmd::StepResponse& event) -> etl::fsm_state_id_t {
        get_fsm_context().last_cmd_data = event.watts;
        return DeviceState_StepResponse;
    }

    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
        switch (event.type) {
            case ButtonEventId::BUTTON_PRESSED_5X:
                return DeviceState_Bonding;

            // Animate long press start
            case ButtonEventId::BUTTON_LONG_PRESS_START:
                DEBUG("Long press start");
                get_fsm_context().showLongPressProgress();
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

    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

Idle idle;

} // namespace

etl::ifsm_state& state_idle = idle;
