#include "app.hpp"
#include "logger.hpp"
#include "app_states/adrc_test.hpp"
#include "app_states/bonding.hpp"
#include "app_states/init.hpp"
#include "app_states/idle.hpp"
#include "app_states/reflow.hpp"
#include "app_states/sensor_bake.hpp"
#include "app_states/step_response.hpp"

App application;

App::App() : etl::fsm(0) {
    button.setEventHandler([this](ButtonEventId event) {
        this->handleButton(event);
    });
}

void App::LogUnknownEvent(const etl::imessage& msg) {
    APP_LOGI("APP: Unknown event! msg id [{}], state id [{}]", msg.get_message_id(), get_state_id());
}

etl::fsm_state_pack<
    Init_State,
    Idle_State,
    Reflow_State,
    SensorBake_State,
    AdrcTest_State,
    StepResponse_State,
    Bonding_State
> app_states;

void App::setup() {
    set_states(app_states);
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