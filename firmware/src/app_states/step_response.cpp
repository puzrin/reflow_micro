#include "app.hpp"
#include "logger.hpp"

namespace {

class StepResponse : public etl::fsm_state<App, StepResponse, DeviceState_StepResponse,
    AppCmd::Stop, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("State => StepResponse");

        auto& heater = get_fsm_context().heater;
        heater.task_start(HISTORY_ID_STEP_RESPONSE);
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

StepResponse stepResponse;

} // namespace

etl::ifsm_state& state_step_response = stepResponse;
