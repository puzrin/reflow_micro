#include "app.hpp"
#include "logger.hpp"

namespace {

class AdrcTest : public etl::fsm_state<App, AdrcTest, DeviceState_AdrcTest,
    AppCmd::Stop, AppCmd::Button, AppCmd::AdrcTest> {
public:
    etl::fsm_state_id_t on_enter_state() override {
        DEBUG("State => AdrcTest");

        auto& app = get_fsm_context();
        auto& heater = app.heater;

        heater.set_temperature(app.last_cmd_data);
        if (!heater.task_start(HISTORY_ID_ADRC_TEST_MODE)) return DeviceState_Idle;

        heater.temperature_control_on();
        return No_State_Change;
    }

    void on_exit_state() override { get_fsm_context().heater.task_stop(); }

    etl::fsm_state_id_t on_event(const AppCmd::Stop& event) { return DeviceState_Idle; }
    etl::fsm_state_id_t on_event(const AppCmd::Button& event) {
        if (event.type == ButtonEventId::BUTTON_PRESSED_1X) return DeviceState_Idle;
        return No_State_Change;
    }
    etl::fsm_state_id_t on_event(const AppCmd::AdrcTest& event) {
        get_fsm_context().heater.set_temperature(event.temperature);
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

AdrcTest adrcTest;

} // namespace

etl::ifsm_state& state_adrc_test = adrcTest;
