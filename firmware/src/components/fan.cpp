#include "fan.hpp"

void Fan::initialize() {
    if (initialized) return;
    initialized = true;

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = timerNum,
        .freq_hz = pwmFrequency,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = fanPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = channelNum,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = timerNum,
        .duty = 0,
        .hpoint = 0,
        .flags = { .output_invert = 0 }
    };
    ledc_channel_config(&channel_conf);
}

void Fan::setSpeed(uint16_t percent) {
    if (!initialized) { initialize(); }

    if (percent > 100) percent = 100;

    uint16_t duty = (percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channelNum, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channelNum);
}
