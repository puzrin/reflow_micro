#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"
#include "lib/button_engine.hpp"

class ButtonDriver : public IButtonDriver {
public:
    auto get() -> bool override {
        static bool init_done = false;
        if (!init_done) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << btnPin),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_ENABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&io_conf);
            init_done = true;
        }
        return gpio_get_level(btnPin) == 0;
    }
private:
    static constexpr gpio_num_t btnPin{GPIO_NUM_9};
};

template <typename Driver>
class Button : public ButtonEngine<Driver> {
public:
    void start() {
        xTaskCreate(
            [](void* params) {
                auto* self = static_cast<Button*>(params);
                while (true) {
                    self->tick(esp_timer_get_time() / 1000);
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }, "button", 1024*4, this, 4, nullptr
        );
    }
};