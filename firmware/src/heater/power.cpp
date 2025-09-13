#include <etl/limits.h>
#include <pd/pd.h>

#include "power.hpp"

pd::Port port;
pd::fusb302::Fusb302RtosHalEsp32 fusb302_hal;
pd::fusb302::Fusb302Rtos driver(port, fusb302_hal);

pd::Task task(port, driver);
pd::DPM dpm(port);
pd::PRL prl(port, driver);
pd::PE pe(port, dpm, prl, driver);
pd::TC tc(port, driver);

Power power;

void Power::setup() {
    task.start(tc, dpm, pe, prl, driver);
}

void Power::tick() {
    if (port.is_attached && power_status != PowerStatus_PwrOff) {
        power_status = PowerStatus_PwrOff;
    }
}

uint32_t Power::get_peak_mv() {
    return peak_mv;
}

uint32_t Power::get_peak_ma() {
    return peak_ma;
}

uint32_t Power::get_duty_millis() {
    return duty_millis;
}

uint32_t Power::get_load_mohm() {
    if (!is_load_valid()) {
        return etl::numeric_limits<uint32_t>::max();
    }
    return peak_mv * 1000 / peak_ma;
}

uint32_t Power::get_max_power_mw() {
    if (!is_load_valid()) {
        return 0;
    }
    return (peak_mv * peak_ma) / 1000;
}
