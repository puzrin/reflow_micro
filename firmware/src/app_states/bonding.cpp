#include "app.hpp"
#include "logger.hpp"
#include "rpc/rpc.hpp"

namespace {

class Bonding : public etl::fsm_state<App, Bonding, DeviceState_Bonding, AppCmd::BondOff> {
public:
    static constexpr int32_t BONDING_PERIOD_MS = 15 * 1000;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        DEBUG("State => Bonding");

        get_fsm_context().showBondingLoop();

        // Enable bonding for 30 seconds
        xTimeoutTimer = xTimerCreate("BondingTimeout", pdMS_TO_TICKS(BONDING_PERIOD_MS), pdFALSE, (void *)0,
            [](TimerHandle_t xTimer){
                (void)xTimer;
                application.receive(AppCmd::BondOff());
            });

        // Ideally, we should check all returned statuses, but who cares...
        if (xTimeoutTimer) { xTimerStart(xTimeoutTimer, 0); }

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

    auto on_event(const AppCmd::BondOff& event) -> etl::fsm_state_id_t {
        (void)event;
        return DeviceState_Idle;
    }

    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }

private:
    TimerHandle_t xTimeoutTimer{nullptr};
};

Bonding bonding;

} // namespace

etl::ifsm_state& state_bonding = bonding;
