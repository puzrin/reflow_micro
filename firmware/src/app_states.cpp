#include "app.hpp"
#include "rpc/rpc.hpp"
#include "blink_signals.hpp"
#include "logger.hpp"
#include "proto/generated/types.pb.h"

namespace {

class Init : public etl::fsm_state<App, Init, DeviceState_Init> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Init state entered");
        return DeviceState_Idle;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};


class Idle : public etl::fsm_state<App, Idle, DeviceState_Idle,
    AppCmd::Reflow, AppCmd::SensorBake, AppCmd::AdrcTest, AppCmd::StepResponse, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Idle state entered");
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event(const AppCmd::Reflow& event) {
        auto& heater = get_fsm_context().heater;
        if (!heater.is_hotplate_connected()) return No_State_Change;

        return DeviceState_Reflow;
    }
    etl::fsm_state_id_t on_event(const AppCmd::SensorBake& event) {
        auto& heater = get_fsm_context().heater;
        if (!heater.is_hotplate_connected()) return No_State_Change;

        heater.set_power(event.watts);
        return DeviceState_SensorBake;
    }
    etl::fsm_state_id_t on_event(const AppCmd::AdrcTest& event) {
        auto& heater = get_fsm_context().heater;
        if (!heater.is_hotplate_connected()) return No_State_Change;

        heater.set_temperature(event.temperature);
        return DeviceState_AdrcTest;
    }
    etl::fsm_state_id_t on_event(const AppCmd::StepResponse& event) {
        auto& heater = get_fsm_context().heater;
        if (!heater.is_hotplate_connected()) return No_State_Change;

        heater.set_power(event.watts);
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


class Reflow : public etl::fsm_state<App, Reflow, DeviceState_Reflow,
    AppCmd::Stop, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Reflow state entered");

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

class SensorBake : public etl::fsm_state<App, SensorBake, DeviceState_SensorBake,
    AppCmd::Stop, AppCmd::Button, AppCmd::SensorBake> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("SensorBake state entered");

        auto& heater = get_fsm_context().heater;
        heater.task_start(HISTORY_ID_SENSOR_BAKE_MODE);
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

class AdrcTest : public etl::fsm_state<App, AdrcTest, DeviceState_AdrcTest,
    AppCmd::Stop, AppCmd::Button, AppCmd::AdrcTest> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("AdrcTest state entered");

        auto& heater = get_fsm_context().heater;
        heater.task_start(HISTORY_ID_ADRC_TEST_MODE);
        heater.temperature_control_on();
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
    etl::fsm_state_id_t on_event(const AppCmd::AdrcTest& event) {
        get_fsm_context().heater.set_temperature(event.temperature);
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

class StepResponse : public etl::fsm_state<App, StepResponse, DeviceState_StepResponse,
    AppCmd::Stop, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("StepResponse state entered");

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


class Bonding : public etl::fsm_state<App, Bonding, DeviceState_Bonding, AppCmd::BondOff> {
public:
    static constexpr uint32_t BONDING_PERIOD_MS = 15*1000;

    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Bonding state entered");

        BLINK_BONDING_LOOP(get_fsm_context().blinker);

        // Enable bonding for 30 seconds
        xTimeoutTimer = xTimerCreate("BondingTimeout", pdMS_TO_TICKS(BONDING_PERIOD_MS), pdFALSE, (void *)0,
            [](TimerHandle_t xTimer){ app.receive(AppCmd::BondOff()); });

        // Ideally, we should check all returned statuses, but who cares...
        if (xTimeoutTimer) xTimerStart(xTimeoutTimer, 0);

        pairing_enable();
        return No_State_Change;
    }

    void on_exit_state() {
        if (xTimeoutTimer) {
            xTimerStop(xTimeoutTimer, 0);
            xTimerDelete(xTimeoutTimer, 0);
            xTimeoutTimer = nullptr;
        }
        pairing_disable();
        get_fsm_context().blinker.off();
    }

    etl::fsm_state_id_t on_event(const AppCmd::BondOff& event) { return DeviceState_Idle; }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }

private:
    TimerHandle_t xTimeoutTimer = nullptr;
};


Init init;
Idle idle;
Reflow reflow;
SensorBake sensorBake;
AdrcTest adrcTest;
StepResponse stepResponse;
Bonding bonding;

etl::ifsm_state* stateList[DeviceState_NumberOfStates] = {
    &init,
    &idle,
    &reflow,
    &sensorBake,
    &adrcTest,
    &stepResponse,
    &bonding
};

constexpr bool check_device_state_sequential() {
    size_t i = 0;
    for(auto state : {
        DeviceState_Init,
        DeviceState_Idle,
        DeviceState_Reflow,
        DeviceState_SensorBake,
        DeviceState_AdrcTest,
        DeviceState_StepResponse,
        DeviceState_Bonding,
        DeviceState_NumberOfStates
    }) {
        if(static_cast<int>(state) != i++) return false;
    }
    return true;
}

}

void app_setup_states(App& app) {
    static_assert(check_device_state_sequential(), "DeviceState values must be sequential starting from 0");
    app.set_states(stateList, DeviceState_NumberOfStates);
}