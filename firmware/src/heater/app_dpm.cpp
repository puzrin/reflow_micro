#include "app_dpm.hpp"
#include "power.hpp"
#include "profile_selector.hpp"

void AppDPM::setup() {
    port.dpm_rtr = &power.dpm_event_listener;
}

void AppDPM::clear_trigger_to(uint32_t pos, uint32_t mv) {
    // By default try 6V (if PPS available). It will fallback to 5V otherwise.
    // This method is used to make changes without triggering new power level request.
    trigger_mv = mv;
    trigger_ma = 0;
    trigger_pdo_variant = pd::PDO_VARIANT::UNKNOWN;
    trigger_position = pos;
    trigger_match_type = TRIGGER_MATCH_TYPE::BY_POSITION;
}
