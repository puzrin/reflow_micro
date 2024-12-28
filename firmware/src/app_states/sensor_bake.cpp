#include "app.hpp"
#include "logger.hpp"

namespace {

class SensorBake : public etl::fsm_state<App, SensorBake, DeviceState_SensorBake,
    AppCmd::Stop, AppCmd::Button, AppCmd::SensorBake> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("State => SensorBake");

        auto& app = get_fsm_context();
        auto& heater = app.heater;
        if (!heater.task_start(HISTORY_ID_SENSOR_BAKE_MODE)) return DeviceState_Idle;

        heater.set_power(app.last_cmd_data);

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
    etl::fsm_state_id_t on_event(const AppCmd::SensorBake& event) {
        get_fsm_context().heater.set_power(event.watts);
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

SensorBake sensorBake;

} // namespace

etl::ifsm_state& state_sensor_bake = sensorBake;
