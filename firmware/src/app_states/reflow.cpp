#include "app.hpp"
#include "logger.hpp"

namespace {

class Reflow : public etl::fsm_state<App, Reflow, DeviceState_Reflow,
    AppCmd::Stop, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("State => Reflow");

        auto& app = get_fsm_context();
        auto& heater = app.heater;
        app.blinker.once({ {0, 200}, {255, 300}, {0, 200} });

        // TODO: add profile loading and temperature generator
        const int32_t profile = 0;
        heater.temperature_control_on();
        heater.task_start(profile);
        return No_State_Change;
    }

    void on_exit_state() {
        get_fsm_context().heater.task_stop();
    }

    etl::fsm_state_id_t on_event(const AppCmd::Stop& event) { return DeviceState_Idle; }
    etl::fsm_state_id_t on_event(const AppCmd::Button& event) {
        if (event.type == ButtonEventId::BUTTON_PRESSED_1X) return DeviceState_Idle;
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

Reflow reflow;

} // namespace

etl::ifsm_state& state_reflow = reflow;
