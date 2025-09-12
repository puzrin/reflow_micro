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

void Power::setup() {
    task.start(tc, dpm, pe, prl, driver);
}