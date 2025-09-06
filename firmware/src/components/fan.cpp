#include "fan.hpp"

Fan fan;

void Fan::initialize() {
    if (initialized) return;
    initialized = true;

    ledc_timer_config_t timer_conf{};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.timer_num = timerNum;
    timer_conf.freq_hz = pwmFrequency;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf{};
    channel_conf.gpio_num = fanPin;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = channelNum;
    channel_conf.timer_sel = timerNum;
    channel_conf.duty = 0;
    ledc_channel_config(&channel_conf);
}

void Fan::setSpeed(uint16_t percent) {
    if (!initialized) { initialize(); }

    if (percent > 100) percent = 100;

    uint16_t duty = (percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channelNum, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channelNum);
}
