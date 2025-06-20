#include "app.hpp"
#include "logger.hpp"
#include "blink_signals.hpp"

static constexpr etl::message_router_id_t APP_FSM_ROUTER_ID = 0;
App application;

App::App() : etl::fsm(APP_FSM_ROUTER_ID) {
    button.setEventHandler([this](ButtonEventId event) {
        this->handleButton(event);
    });
}

void App::LogUnknownEvent(const etl::imessage& msg) {
    DEBUG("APP: Unknown event! msg id [{}], state id [{}]", msg.get_message_id(), get_state_id());
}

extern etl::ifsm_state& state_init;
extern etl::ifsm_state& state_idle;
extern etl::ifsm_state& state_reflow;
extern etl::ifsm_state& state_sensor_bake;
extern etl::ifsm_state& state_adrc_test;
extern etl::ifsm_state& state_step_response;
extern etl::ifsm_state& state_bonding;

namespace {

etl::ifsm_state* stateList[DeviceState_NumberOfStates] = {
    &state_init,
    &state_idle,
    &state_reflow,
    &state_sensor_bake,
    &state_adrc_test,
    &state_step_response,
    &state_bonding
};

constexpr auto check_device_state_sequential() -> bool {
    size_t i = 0;
    for(const auto state : {
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

void App::setup() {
    static_assert(check_device_state_sequential(), "DeviceState values must be sequential starting from 0");
    set_states(stateList, DeviceState_NumberOfStates);
    start();

    heater.start();
    button.start();
    blinker.start();
    fan.setSpeed(0);
    BLINK_SET_IDLE_BACKGROUND(blinker);
}
