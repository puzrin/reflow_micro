#pragma once

//
// PWM / Timers usage:
//
// PWM0 / T0: LED (demo board only, real device uses RMT)
// PWM1 / T1: Fan speed
// PWM2+PWM3 / T2: Buzzer
//

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "lib/blinker_engine.hpp"

#ifdef HW_DEMO_ESP32_C3_SUPERMINI

#include "driver/ledc.h"

class LedDriver : public IBlinkerLED<1> {
private:
    static constexpr int ledPin{GPIO_NUM_8};
    static constexpr ledc_timer_t timerNum{LEDC_TIMER_0};
    static constexpr ledc_channel_t channelNum{LEDC_CHANNEL_0};

public:
    LedDriver() {
        ledc_timer_config_t timer_conf = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = timerNum,
            .freq_hz = 5000,
            .clk_cfg = LEDC_AUTO_CLK,
            .deconfigure = false
        };
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t ch_conf = {
            .gpio_num = ledPin,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = channelNum,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = timerNum,
            .duty = 0,
            .hpoint = 0,
            .flags = {
                // LED output should be inverted
                .output_invert = 1
            }
        };
        ledc_channel_config(&ch_conf);
    }

    void set(const DataType& value) override {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, channelNum, value[0]);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, channelNum);
    }
};

#else

#include <rmt_led_strip.hpp>

class LedDriver : public IBlinkerLED<3> {
private:
    static constexpr gpio_num_t ledPin{GPIO_NUM_0};

    htcw::ws2812 led{ledPin, 1};
    DataType buffer{};

public:
    using DataType = typename IBlinkerLED<3>::DataType;

    LedDriver() {
        led.initialize();
    }

    void set(const DataType &val) override {
        if (val == buffer) return;
        buffer = val; // persist for async transfer

        led.color(0, buffer[0], buffer[1], buffer[2]);
        led.update();
    }
};

#endif // HW_DEMO_ESP32_C3_SUPERMINI

template <typename Driver>
class Blinker : public BlinkerEngine<Driver> {
private:
    TimerHandle_t timer;

    static void timerCallback(TimerHandle_t timer) {
        Blinker* self = static_cast<Blinker*>(pvTimerGetTimerID(timer));
        self->tick(esp_timer_get_time() / 1000);
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