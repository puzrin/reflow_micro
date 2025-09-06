#pragma once

#include "etl/fsm.h"
#include "heater_mock.hpp"
#include "components/button.hpp"
#include "components/blinker.hpp"
#include "components/buzzer.hpp"
#include "components/fan.hpp"
#include "components/profiles_config.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace AppCmd {

namespace _id {
    enum {
        STOP,
        SUCCEEDED,
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
DEFINE_SIMPLE_MSG(Succeeded, _id::SUCCEEDED);
DEFINE_SIMPLE_MSG(Reflow, _id::REFLOW);
DEFINE_PARAM_MSG(SensorBake, _id::SENSOR_BAKE, float, watts);
DEFINE_PARAM_MSG(AdrcTest, _id::ADRC_TEST, float, temperature);
DEFINE_PARAM_MSG(StepResponse, _id::STEP_RESPONSE, float, watts);
DEFINE_SIMPLE_MSG(BondOff, _id::BOND_OFF);
DEFINE_PARAM_MSG(Button, _id::BUTTON, ButtonEventId, type);

using Packet = etl::message_packet<
    AppCmd::Stop,
    AppCmd::Succeeded,
    AppCmd::Reflow,
    AppCmd::SensorBake,
    AppCmd::AdrcTest,
    AppCmd::StepResponse,
    AppCmd::BondOff,
    AppCmd::Button
>;

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

    void receive(const etl::imessage& message) {
        xSemaphoreTake(message_lock, portMAX_DELAY);
        etl::fsm::receive(message);
        xSemaphoreGive(message_lock);
    }

    // Asynchronous message delivery. When sync processing not required OR
    // message is generated from state (from inside of .receive()).
    // It also guarantees processing priority.
    template <typename TMessage>
    void enqueue_message(const TMessage& message) {
        AppCmd::Packet packet(message);
        xQueueSend(message_queue, &packet, 0);
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
    SemaphoreHandle_t message_lock{xSemaphoreCreateMutex()};
    QueueHandle_t message_queue{nullptr};
    void message_consumer_loop();

    Button<ButtonDriver> button{};
    void handleButton(ButtonEventId event) {
        if (event == ButtonEventId::BUTTON_PRESS_START) {
            beepButtonPress();
        }
        enqueue_message(AppCmd::Button{event});
    }
};

extern App application;
