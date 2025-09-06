#pragma once

//
// PWM / Timers usage:
//
// PWM0 / T0: LED (demo board only, real device uses RMT)
// PWM1 / T1: Fan speed
// PWM2+PWM3 / T2: Buzzer
//

#include "driver/ledc.h"

class Fan {
private:
    static constexpr int fanPin{GPIO_NUM_21};
    static constexpr int pwmFrequency{25000};
    static constexpr ledc_timer_t timerNum{LEDC_TIMER_1};
    static constexpr ledc_channel_t channelNum{LEDC_CHANNEL_1};

    bool initialized{false};

    void initialize();

public:
    void setSpeed(uint16_t percent);

    inline void off() { setSpeed(0); }

    inline void max() { setSpeed(100); }
};

extern Fan fan;
