#include "app.hpp"
#include "logger.hpp"
#include "blink_signals.hpp"
#include "rpc/rpc.hpp"

namespace {

class Bonding : public etl::fsm_state<App, Bonding, DeviceState_Bonding, AppCmd::BondOff> {
public:
    static constexpr int32_t BONDING_PERIOD_MS = 15 * 1000;

    etl::fsm_state_id_t on_enter_state() override {
        DEBUG("State => Bonding");

        BLINK_BONDING_LOOP(get_fsm_context().blinker);

        // Enable bonding for 30 seconds
        xTimeoutTimer = xTimerCreate("BondingTimeout", pdMS_TO_TICKS(BONDING_PERIOD_MS), pdFALSE, (void *)0,
            [](TimerHandle_t xTimer){ application.receive(AppCmd::BondOff()); });

        // Ideally, we should check all returned statuses, but who cares...
        if (xTimeoutTimer) xTimerStart(xTimeoutTimer, 0);

        pairing_enable();
        return No_State_Change;
    }

    void on_exit_state() override {
        if (xTimeoutTimer) {
            xTimerStop(xTimeoutTimer, 0);
            xTimerDelete(xTimeoutTimer, 0);
            xTimeoutTimer = nullptr;
        }
        pairing_disable();
        get_fsm_context().blinker.off();
    }

    etl::fsm_state_id_t on_event(const AppCmd::BondOff& event) { return DeviceState_Idle; }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }

private:
    TimerHandle_t xTimeoutTimer = nullptr;
};

Bonding bonding;

} // namespace

etl::ifsm_state& state_bonding = bonding;
