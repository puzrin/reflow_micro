#pragma once

#include <etl/atomic.h>

#include "proto/generated/types.pb.h"

class Power {
public:
    void setup();

    uint32_t get_peak_mv();
    uint32_t get_peak_ma();
    uint32_t get_duty_millis();
    uint32_t get_load_mohm();
    uint32_t get_max_power_mw();
    uint32_t is_load_valid() { return peak_ma >= 10 && peak_mv >= 10; };
    PowerStatus get_power_status() { return power_status; }

private:
    void tick();

    etl::atomic<uint32_t> peak_mv{0};
    etl::atomic<uint32_t> peak_ma{0};
    etl::atomic<uint32_t> duty_millis{0};

    etl::atomic<uint32_t> power_setpoint_mw{0};
    etl::atomic<PowerStatus> power_status{PowerStatus_PwrOff};
};

extern Power power;

