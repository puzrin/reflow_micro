#include "app.hpp"
#include "logger.hpp"

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
    showIdleBackground();

    // Temporary
    showLedTest();
    beepReflowComplete();
}

void App::showIdleBackground() {
    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    blinker.background({10});
    #else
    blinker.background({0, 100, 0});
    #endif
}

void App::showLongPressProgress() {
    int animation_time = ButtonConstants::LONG_PRESS_THRESHOLD - ButtonConstants::SHORT_PRESS_THRESHOLD;

    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    blinker.once({{10, 0}, blinker.flowTo(255, animation_time)});
    #else
    blinker.once({{{0, 100, 0}, 0}, blinker.flowTo({255, 255, 255}, animation_time)});
    #endif
}

void App::showBondingLoop() {
    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    blinker.loop({{10, 150}, {0, 250}});
    #else
    blinker.loop({{{0, 0, 255}, 150}, {{0, 0, 0}, 250}});
    #endif
}

void App::showReflowStart() {
    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    blinker.once({{0, 200}, {255, 300}, {0, 200}});
    #else
    blinker.once({{{0, 0, 0}, 200}, {{0, 255, 0}, 300}, {{0, 0, 0}, 200}});
    #endif
}

void App::showLedTest() {
    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    // Nothing at demo board
    #else
    // R / G / B / W cycle
    blinker.loop({{{255, 0, 0}, 1000}, {{0, 255, 0}, 1000}, {{0, 0, 255}, 1000}, {{128, 128, 128}, 1000}});
    #endif
}

void App::beepButtonPress() {
    buzzer.play(":b=300:64e7"_rtttl2tones);
}

void App::beepReflowStarted() {
    buzzer.play(":d=32,o=6,b=200:c,g#"_rtttl2tones);
}

void App::beepReflowComplete() {
    buzzer.play(":d=32,o=5,b=300:c6,e6,g6,b6,8c7,c6,e6,g6,b6,8c7"_rtttl2tones);
}

void App::beepReflowTerminated() {
    buzzer.play(":d=32,o=6,b=200:g,f#,f,e,d#,8d"_rtttl2tones);
}