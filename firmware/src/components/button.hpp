#pragma once

#include <Arduino.h>
#include "lib/button_engine.hpp"

class ButtonDriver : public IButtonDriver {
public:
    ButtonDriver() : initialized(false) {};
    bool get() override {
        if (!initialized) {
            initialized = true;
            pinMode(btnPin, INPUT_PULLUP);
        }
        return digitalRead(btnPin) == LOW;
    }
private:
    static constexpr uint8_t btnPin = 9;
    bool initialized;
};

template <typename Driver>
class Button : public ButtonEngine<Driver> {
private:
    TimerHandle_t timer;

    static void timerCallback(TimerHandle_t timer) {
        Button* self = static_cast<Button*>(pvTimerGetTimerID(timer));
        self->tick(millis());
    }

public:
    void start() {
        timer = xTimerCreate(
            "ButtonTimer",
            pdMS_TO_TICKS(10),
            pdTRUE,
            this,
            timerCallback
        );
        xTimerStart(timer, 0);
    }
};
