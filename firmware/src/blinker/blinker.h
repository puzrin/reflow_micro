#pragma once

#include <Arduino.h>
#include "blinker_engine.hpp"

class LedDriver : public IBlinkerLED<1> {
public:
    LedDriver() : initialized(false) {}

    void set(const DataType& value) override {
        if (!initialized) {
            initialized = true;
            pinMode(ledPin, OUTPUT);
        }
        analogWrite(ledPin, 255 - value[0]);  // Write inverted PWM to GP8
    }
private:
    static constexpr uint8_t ledPin = 8;
    bool initialized;
};

using Blinker = BlinkerEngine<LedDriver>;

extern Blinker blinker;
void blinker_init();
