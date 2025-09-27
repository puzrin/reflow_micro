#include "app_dpm.hpp"
#include "power.hpp"
#include "profile_selector.hpp"

void AppDPM::setup() {
    port.dpm_rtr = &power.dpm_event_listener;
}

void AppDPM::clear_trigger() {
    // By default try 6V (if PPS available). It will fallback to 5V otherwise.
    // This method is used to make changes without triggering new power level request.
    trigger_mv = Power::DEFAULT_VOLTAGE_MV;
    trigger_ma = 0;
    trigger_pdo_variant = pd::PDO_VARIANT::UNKNOWN;
    trigger_position = 0;
    trigger_match_type = TRIGGER_MATCH_TYPE::USE_ANY;
}
