#pragma once

#include <Arduino.h>
#include "lib/blinker_engine.hpp"

class LedDriver : public IBlinkerLED<1> {
public:
    void set(const DataType& value) override {
        if (!initialized) {
            initialized = true;
            pinMode(ledPin, OUTPUT);
        }
        analogWrite(ledPin, 255 - value[0]);  // Write inverted PWM to GP8
    }
private:
    static constexpr uint8_t ledPin{8};
    bool initialized{false};
};

template <typename Driver>
class Blinker : public BlinkerEngine<Driver> {
private:
    TimerHandle_t timer;

    static void timerCallback(TimerHandle_t timer) {
        Blinker* self = static_cast<Blinker*>(pvTimerGetTimerID(timer));
        self->tick(millis());
    }

public:
    void start() {
        timer = xTimerCreate(
            "BlinkerTimer",
            pdMS_TO_TICKS(20),
            pdTRUE,
            this,
            timerCallback
        );
        xTimerStart(timer, 0);
    }
};
