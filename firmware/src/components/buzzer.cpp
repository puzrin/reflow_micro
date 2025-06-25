#include "buzzer.hpp"

// ======================== BuzzerDriver ========================

BuzzerDriver::BuzzerDriver() {
    sound(0);
}

uint8_t BuzzerDriver::get_min_pwm_resolution(uint32_t freq_hz) {
    uint8_t min_bits = 4;
    uint8_t max_bits = 13;

    if (freq_hz == 0) return max_bits;

    uint32_t ratio = 80'000'000u / (1024u * freq_hz) + 1;

    int bits = 0;
    while ((1u << bits) < ratio) ++bits;

    if (bits < min_bits) bits = min_bits;
    if (bits > max_bits) bits = max_bits;
    return bits;
}

void BuzzerDriver::sound(uint16_t freq_hz) {
    if (freq_hz == 0) {
        gpio_config_t gpio_conf = {
            .pin_bit_mask = (1ULL << GPIO_PIN_A),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_reset_pin(static_cast<gpio_num_t>(GPIO_PIN_A));
        gpio_config(&gpio_conf);
        gpio_set_level(static_cast<gpio_num_t>(GPIO_PIN_A), 0);

#ifndef HW_DEMO_ESP32_C3_SUPERMINI
        gpio_conf.pin_bit_mask = (1ULL << GPIO_PIN_B);
        gpio_reset_pin(static_cast<gpio_num_t>(GPIO_PIN_B));
        gpio_config(&gpio_conf);
        gpio_set_level(static_cast<gpio_num_t>(GPIO_PIN_B), 0);
#endif
        return;
    }

    auto pwm_bits = get_min_pwm_resolution(freq_hz);

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(pwm_bits),
        .timer_num = LEDC_TIMER_2,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_USE_APB_CLK,
        .deconfigure = false
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_PIN_A,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_2,
        .duty = 1u << (pwm_bits - 1), // 50% duty cycle
        .hpoint = 0,
        .flags = {.output_invert = 0}
    };
    ledc_channel_config(&channel_conf);

#ifndef HW_DEMO_ESP32_C3_SUPERMINI
    channel_conf.gpio_num = GPIO_PIN_B;
    channel_conf.channel = LEDC_CHANNEL_3;
    channel_conf.flags.output_invert = 1; // Invert output for channel B
    ledc_channel_config(&channel_conf);
#endif
}

// ======================== Buzzer ========================

Buzzer::Buzzer() {
    timer_ = xTimerCreate("buzzer_timer",
                         pdMS_TO_TICKS(TICK_INTERVAL_MS),
                         pdTRUE,
                         this,
                         timer_callback);
}

void Buzzer::play(const rtttl::Tone* tones, size_t count) {
    version_.fetch_add(1, std::memory_order_release);

    shadow_tones_ = tones;
    shadow_tone_count_ = count;

    version_.fetch_add(1, std::memory_order_release);

    xTimerStart(timer_, 0);
}

void Buzzer::tick() {
    uint32_t current_version = version_.load(std::memory_order_acquire);

    if (current_version & 1) { return; }

    if (current_version != last_version_) {
        driver_.sound(0);

        active_tones_ = shadow_tones_;
        active_tone_count_ = shadow_tone_count_;

        uint32_t check_version = version_.load(std::memory_order_acquire);
        if (check_version != current_version) { return; }

        last_version_ = current_version;
        tone_index_ = 0;
        start_time_ = xTaskGetTickCount();
        in_gap_ = false;

        if (active_tone_count_ > 0) {
            driver_.sound(active_tones_[0].freq_hz);
        }
        return;
    }

    if (active_tone_count_ == 0) { return; }

    if (tone_index_ >= active_tone_count_) {
        driver_.sound(0);
        tone_index_ = 0;
        active_tone_count_ = 0;
        return;
    }

    TickType_t current_time = xTaskGetTickCount();
    TickType_t elapsed = current_time - start_time_;

    if (in_gap_) {
        if (elapsed >= pdMS_TO_TICKS(NOTE_GAP_MS)) {
            tone_index_++;

            if (tone_index_ >= active_tone_count_) {
                driver_.sound(0);
                return;
            }

            driver_.sound(active_tones_[tone_index_].freq_hz);
            start_time_ = current_time;
            in_gap_ = false;
        }
    } else {
        if (elapsed >= pdMS_TO_TICKS(active_tones_[tone_index_].duration_ms)) {
            driver_.sound(0);
            start_time_ = current_time;
            in_gap_ = true;
        }
    }
}

void Buzzer::timer_callback(TimerHandle_t timer) {
    Buzzer* buzzer = static_cast<Buzzer*>(pvTimerGetTimerID(timer));
    buzzer->tick();
}
