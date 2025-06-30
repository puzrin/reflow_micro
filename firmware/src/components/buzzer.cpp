#include "buzzer.hpp"

// ======================== BuzzerDriver ========================

BuzzerDriver::BuzzerDriver() {
    sound(1000); // Attach PWM to GPIO
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
        ledc_stop(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, IDLE_LEVEL);
        if (doubleOutput) {
            ledc_stop(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, IDLE_LEVEL);
        }
        return;
    }

    auto pwm_bits = get_min_pwm_resolution(freq_hz);

    ledc_timer_config_t timer_conf{};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = static_cast<ledc_timer_bit_t>(pwm_bits);
    timer_conf.timer_num = static_cast<ledc_timer_t>(PWM_TIMER_CHANNEL);
    timer_conf.freq_hz = freq_hz;
    timer_conf.clk_cfg = LEDC_USE_APB_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf{};
    channel_conf.gpio_num = GPIO_PIN_A;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = PWM_CHANNEL_A;
    channel_conf.timer_sel = static_cast<ledc_timer_t>(PWM_TIMER_CHANNEL);
    channel_conf.duty = 1u << (pwm_bits - 1); // 50% duty cycle
    ledc_channel_config(&channel_conf);

    if (doubleOutput) {
        channel_conf.gpio_num = GPIO_PIN_B;
        channel_conf.channel = PWM_CHANNEL_B;
        channel_conf.flags.output_invert = 1; // Invert output for channel B
        ledc_channel_config(&channel_conf);
    }
}

// ======================== Buzzer ========================

Buzzer::Buzzer() {
    timer_ = xTimerCreate("buzzer_timer",
                         pdMS_TO_TICKS(TICK_INTERVAL_MS),
                         pdTRUE,
                         this,
                         timer_callback);
}

void Buzzer::play(const rtttl::ToneSeq& tones) {
    version_.fetch_add(1, std::memory_order_release);

    shadow_tones_ = tones;

    version_.fetch_add(1, std::memory_order_release);

    xTimerStart(timer_, 0);
}

void Buzzer::tick() {
    uint32_t current_version = version_.load(std::memory_order_acquire);

    if (current_version & 1) { return; }

    if (current_version != last_version_) {
        driver_.sound(0);

        active_tones_ = shadow_tones_;

        uint32_t check_version = version_.load(std::memory_order_acquire);
        if (check_version != current_version) { return; }

        last_version_ = current_version;
        tone_index_ = 0;
        start_time_ = xTaskGetTickCount();
        in_gap_ = false;

        if (active_tones_.size > 0) {
            driver_.sound(active_tones_.data[0].freq_hz);
        }
        return;
    }

    if (active_tones_.size == 0) { return; }

    if (tone_index_ >= active_tones_.size) {
        driver_.sound(0);
        tone_index_ = 0;
        active_tones_ = {nullptr, 0};
        return;
    }

    TickType_t current_time = xTaskGetTickCount();
    TickType_t elapsed = current_time - start_time_;

    if (in_gap_) {
        if (elapsed >= pdMS_TO_TICKS(NOTE_GAP_MS)) {
            tone_index_++;

            if (tone_index_ >= active_tones_.size) {
                driver_.sound(0);
                return;
            }

            driver_.sound(active_tones_.data[tone_index_].freq_hz);
            start_time_ = current_time;
            in_gap_ = false;
        }
    } else {
        if (elapsed >= pdMS_TO_TICKS(active_tones_.data[tone_index_].duration_ms)) {
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
