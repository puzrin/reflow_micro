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

#define DEFINE_SIMPLE_MSG(ClassName, MsgId) \
    class ClassName : public etl::message<MsgId> {}

#define DEFINE_PARAM_MSG(ClassName, MsgId, ParamType, ParamName) \
    class ClassName : public etl::message<MsgId> { \
    public: \
        explicit ClassName(ParamType ParamName) : ParamName{ParamName} {} \
        const ParamType ParamName; \
    }

DEFINE_SIMPLE_MSG(Stop, _id::STOP);
DEFINE_SIMPLE_MSG(Reflow, _id::REFLOW);
DEFINE_PARAM_MSG(SensorBake, _id::SENSOR_BAKE, float, watts);
DEFINE_PARAM_MSG(AdrcTest, _id::ADRC_TEST, float, temperature);
DEFINE_PARAM_MSG(StepResponse, _id::STEP_RESPONSE, float, watts);
DEFINE_SIMPLE_MSG(BondOff, _id::BOND_OFF);
DEFINE_PARAM_MSG(Button, _id::BUTTON, ButtonEventId, type);

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
