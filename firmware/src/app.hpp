#pragma once

#include "etl/fsm.h"
#include "heater_mock.hpp"
#include "lib/sparse_history.hpp"
#include "components/button.hpp"
#include "components/blinker.hpp"

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
class SensorBake : public etl::message<_id::SENSOR_BAKE> {};
class AdrcTest : public etl::message<_id::ADRC_TEST> {};
class StepResponse : public etl::message<_id::STEP_RESPONSE> {};
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
    SparseHistory history;
    Blinker<LedDriver> blinker;

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

void app_setup_states(App& app);
