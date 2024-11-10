#pragma once

#include <Arduino.h>
#include "button_engine.hpp"

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

using Button = ButtonEngine<ButtonDriver>;

extern Button button;
void button_init();
