#pragma once

#include <Arduino.h>
#include "lib/button_engine.hpp"

class ButtonDriver : public IButtonDriver {
public:
    auto get() -> bool override {
        if (!initialized) {
            initialized = true;
            pinMode(btnPin, INPUT_PULLUP);
        }
        return digitalRead(btnPin) == LOW;
    }
private:
    static constexpr uint8_t btnPin{9};
    bool initialized{false};
};

template <typename Driver>
class Button : public ButtonEngine<Driver> {
public:
    void start() {
        xTaskCreate(
            [](void* params) {
                auto* self = static_cast<Button*>(params);
                while (true) {
                    self->tick(millis());
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }, "button", 1024*4, this, 4, nullptr
        );
    }
};
