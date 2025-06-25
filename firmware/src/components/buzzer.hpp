#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "lib/rtttl.hpp"

class BuzzerDriver {
public:
    static constexpr int PWM_TIMER_CHANNEL = LEDC_CHANNEL_2;
    static constexpr uint8_t GPIO_PIN_A = 10;
    static constexpr ledc_channel_t PWM_CHANNEL_A = LEDC_CHANNEL_2;
    static constexpr uint8_t GPIO_PIN_B = 8;
    static constexpr ledc_channel_t PWM_CHANNEL_B = LEDC_CHANNEL_3;

    #ifdef HW_DEMO_ESP32_C3_SUPERMINI
    static constexpr bool doubleOutput = false;
    static constexpr uint8_t IDLE_LEVEL = 0;
    #else
    static constexpr bool doubleOutput = true;
    static constexpr uint8_t IDLE_LEVEL = 0;
    #endif

    BuzzerDriver();

    void sound(uint16_t freq_hz);

private:
    void set_duty(uint8_t duty_percent);
    uint8_t get_min_pwm_resolution(uint32_t freq_hz);
};

class Buzzer {
public:
    static constexpr uint32_t NOTE_GAP_MS = 5;
    static constexpr uint32_t TICK_INTERVAL_MS = 1;

    Buzzer();

    void play(const rtttl::ToneSeq& tones);

private:
    std::atomic<uint32_t> version_{0};

    rtttl::ToneSeq shadow_tones_{nullptr, 0};
    rtttl::ToneSeq active_tones_{nullptr, 0};

    size_t tone_index_{0};
    TickType_t start_time_{0};
    bool in_gap_{false};

    BuzzerDriver driver_;
    TimerHandle_t timer_;
    uint32_t last_version_{0};

    void tick();

    static void timer_callback(TimerHandle_t timer);
};