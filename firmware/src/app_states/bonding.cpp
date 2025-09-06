#include "bonding.hpp"
#include "logger.hpp"
#include "rpc/rpc.hpp"

auto Bonding_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => Bonding");

    get_fsm_context().showBondingLoop();

    // Enable bonding for 30 seconds
    xTimeoutTimer = xTimerCreate("BondingTimeout", pdMS_TO_TICKS(BONDING_PERIOD_MS), pdFALSE, (void *)0,
        [](TimerHandle_t){
            application.enqueue_message(AppCmd::BondOff{});
        });

    // Ideally, we should check all returned statuses, but who cares...
    if (xTimeoutTimer) { xTimerStart(xTimeoutTimer, 0); }

    pairing_enable();
    return No_State_Change;
}

auto Bonding_State::on_event(const AppCmd::BondOff&) -> etl::fsm_state_id_t {
    return DeviceState_Idle;
}

auto Bonding_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) {
        // Exit bonding on single press
        return DeviceState_Idle;
    }
    return No_State_Change;
}

auto Bonding_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void Bonding_State::on_exit_state() {
    if (xTimeoutTimer) {
        xTimerStop(xTimeoutTimer, 0);
        xTimerDelete(xTimeoutTimer, 0);
        xTimeoutTimer = nullptr;
    }
    pairing_disable();
    get_fsm_context().blinker.off();
}
