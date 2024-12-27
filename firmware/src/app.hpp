#pragma once

#include "etl/fsm.h"
#include "heater_mock.hpp"
#include "components/button.hpp"
#include "components/blinker.hpp"
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
    SensorBake(float watts) : watts(watts) {}
    float watts;
};

class AdrcTest : public etl::message<_id::ADRC_TEST> {
public:
    AdrcTest(float temperature) : temperature(temperature) {}
    float temperature;
};

class StepResponse : public etl::message<_id::STEP_RESPONSE> {
public:
    StepResponse(float watts) : watts(watts) {}
    float watts;
};

class BondOff : public etl::message<_id::BOND_OFF> {};

class Button : public etl::message<_id::BUTTON> {
public:
    Button(ButtonEventId type) : type(type) {}
    ButtonEventId type;
};

} // namespace AppCmd

class App : public etl::fsm {
public:
    App();

    HeaterMock heater;
    Blinker<LedDriver> blinker;
    ProfilesConfig profilesConfig;

    void LogUnknownEvent(const etl::imessage& msg);
    void setup();

    void safe_receive(const etl::imessage& message) {
        if (!mutex) mutex = xSemaphoreCreateMutex();
        xSemaphoreTake(mutex, portMAX_DELAY);
        receive(message);
        xSemaphoreGive(mutex);
    }

private:
    SemaphoreHandle_t mutex = nullptr;
    Button<ButtonDriver> button;
    void handleButton(ButtonEventId event) { safe_receive(AppCmd::Button(event)); }
};

extern App app;
