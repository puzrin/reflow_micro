#include "buzzer.hpp"
#include "time.hpp"

Buzzer buzzer;

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

void BuzzerDriver::set_duty(uint32_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A);
    if (doubleOutput) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B);
    }
}

void BuzzerDriver::sound(uint16_t freq_hz) {
    if (!initialized) {
        // - Set channels only once to avoid warnings.
        // - Any valid timer required for channel(s) setup.
        ledc_timer_config_t t{};
        t.speed_mode      = LEDC_LOW_SPEED_MODE;
        t.timer_num       = (ledc_timer_t)PWM_TIMER_CHANNEL;
        t.duty_resolution = LEDC_TIMER_10_BIT;   // any
        t.freq_hz         = 1000;                // any
        t.clk_cfg         = LEDC_USE_APB_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&t));

        ledc_channel_config_t ch{};
        ch.speed_mode = LEDC_LOW_SPEED_MODE;
        ch.timer_sel  = (ledc_timer_t)PWM_TIMER_CHANNEL;
        ch.duty       = 0;
        ch.channel    = PWM_CHANNEL_A;
        ch.gpio_num   = GPIO_PIN_A;
        ESP_ERROR_CHECK(ledc_channel_config(&ch));

        if (doubleOutput) {
            ch.channel  = PWM_CHANNEL_B;
            ch.gpio_num = GPIO_PIN_B;
            ch.flags.output_invert = 1;
            ESP_ERROR_CHECK(ledc_channel_config(&ch));
        }

        initialized = true;
    }

    if (freq_hz == 0) {
        ledc_stop(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_A, IDLE_LEVEL);
        if (doubleOutput) {
            // Output is inverted (see below)
            ledc_stop(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_B, IDLE_LEVEL ? 0 : 1);
        }
        return;
    }

    auto pwm_bits = get_min_pwm_resolution(freq_hz);
    set_duty(0); // dim click

    ledc_timer_config_t t{};
    t.speed_mode = LEDC_LOW_SPEED_MODE;
    t.duty_resolution = static_cast<ledc_timer_bit_t>(pwm_bits);
    t.timer_num = static_cast<ledc_timer_t>(PWM_TIMER_CHANNEL);
    t.freq_hz = freq_hz;
    t.clk_cfg = LEDC_USE_APB_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    set_duty(1u << (pwm_bits - 1)); // 50%
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
        start_time_ = Time::now();
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

    if (in_gap_) {
        if (Time(start_time_).expired(NOTE_GAP_MS)) {
            tone_index_++;

            if (tone_index_ >= active_tones_.size) {
                driver_.sound(0);
                return;
            }

            driver_.sound(active_tones_.data[tone_index_].freq_hz);
            start_time_ = Time::now();
            in_gap_ = false;
        }
    } else {
        if (Time(start_time_).expired(active_tones_.data[tone_index_].duration_ms)) {
            driver_.sound(0);
            start_time_ = Time::now();
            in_gap_ = true;
        }
    }
}

void Buzzer::timer_callback(TimerHandle_t timer) {
    Buzzer* buzzer = static_cast<Buzzer*>(pvTimerGetTimerID(timer));
    buzzer->tick();
}
