#pragma once

#include "etl/fsm.h"
#include "heater_mock.hpp"
#include "components/button.hpp"
#include "components/blinker.hpp"
#include "components/buzzer.hpp"
#include "components/fan.hpp"
#include "components/profiles_config.hpp"

namespace AppCmd {

namespace _id {
    enum {
        STOP,
        REFLOW,
        SENSOR_BAKE,
        ADRC_TEST,
        STEP_RESPONSE,
        BOND_OFF,
        BUTTON
    };
}

class Stop : public etl::message<_id::STOP> {};

class Reflow : public etl::message<_id::REFLOW> {};

class SensorBake : public etl::message<_id::SENSOR_BAKE> {
public:
    explicit SensorBake(float watts) : watts{watts} {}
    const float watts;
};

class AdrcTest : public etl::message<_id::ADRC_TEST> {
public:
    explicit AdrcTest(float temperature) : temperature{temperature} {}
    const float temperature;
};

class StepResponse : public etl::message<_id::STEP_RESPONSE> {
public:
    explicit StepResponse(float watts) : watts{watts} {}
    const float watts;
};

class BondOff : public etl::message<_id::BOND_OFF> {};

class Button : public etl::message<_id::BUTTON> {
public:
    explicit Button(ButtonEventId type) : type{type} {}
    const ButtonEventId type;
};

} // namespace AppCmd

class App : public etl::fsm {
public:
    App();

    HeaterMock heater{};
    Blinker<LedDriver> blinker{};
    Buzzer buzzer{};
    ProfilesConfig profilesConfig{};
    Fan fan{};

    void LogUnknownEvent(const etl::imessage& msg);
    void setup();

    void safe_receive(const etl::imessage& message) {
        if (!mutex) { mutex = xSemaphoreCreateMutex(); }
        xSemaphoreTake(mutex, portMAX_DELAY);
        receive(message);
        xSemaphoreGive(mutex);
    }

    float last_cmd_data{0};

    // UI signals
    void showIdleBackground();
    void showLongPressProgress();
    void showBondingLoop();
    void showReflowStart();
    void showLedTest();

    void beepButtonPress();
    void beepReflowStarted();
    void beepReflowComplete();
    void beepReflowTerminated();

private:
    SemaphoreHandle_t mutex{nullptr};
    Button<ButtonDriver> button{};
    void handleButton(ButtonEventId event) {
        if (event == ButtonEventId::BUTTON_PRESS_START) {
            beepButtonPress();
        }
        safe_receive(AppCmd::Button(event));
    }
};

extern App application;
