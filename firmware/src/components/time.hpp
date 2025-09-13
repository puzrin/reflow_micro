#pragma once

#include <stdint.h>
#include <etl/platform.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Time {
public:
    static uint32_t now() {
        static_assert(sizeof(TickType_t) == 4, "Assumes 32-bit FreeRTOS ticks");

        TickType_t t = xPortInIsrContext() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
        return pdTICKS_TO_MS(t);
    };

    Time() : t0_ms(now()) {}
    explicit Time(uint32_t value) : t0_ms(value) {}

    ETL_NODISCARD bool expired(uint32_t timeout_ms) const {
        uint32_t deadline = t0_ms + timeout_ms;
        return static_cast<int32_t>(now() - deadline) >= 0;
    }

private:
    const uint32_t t0_ms;
};
