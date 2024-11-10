#include "app.hpp"
#include "rpc/rpc.hpp"
#include "blink_signals.hpp"
#include "logger.hpp"

namespace {

struct AppStateId {
    enum Enum {
        INIT,
        IDLE,
        WORKING,
        BONDING,
        NUMBER_OF_STATES
    };
};


class Init : public etl::fsm_state<App, Init, AppStateId::INIT> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Init entered");
        //const LedDriver::DataType bg = BLINK_IDLE_BACKGROUND;
        //blinker.background(bg);
        blinker.background(BLINK_IDLE_BACKGROUND);
        DEBUG("Init background set");
        return AppStateId::IDLE;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return STATE_ID;
    } 
};


class Idle : public etl::fsm_state<App, Idle, AppStateId::IDLE, ButtonAction, Start, BondOn> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("Idle entered");
        return STATE_ID;
    }

    etl::fsm_state_id_t on_event(const Start& event) { return AppStateId::WORKING; }
    etl::fsm_state_id_t on_event(const BondOn& event) { return AppStateId::BONDING; }

    etl::fsm_state_id_t on_event(const ButtonAction& event) {
        switch (event.type) {
            case ButtonEventId::BUTTON_PRESSED_5X:
                return AppStateId::BONDING;

            // Animate long press start
            case ButtonEventId::BUTTON_LONG_PRESS_START:
                DEBUG("Long press start");
                blinker.once(BLINK_LONG_PRESS_START);
                break;
            // Stops animation if long press not reached
            case ButtonEventId::BUTTON_LONG_PRESS_FAIL:
                DEBUG("Long press fail");
                blinker.off();
                break;
            
            case ButtonEventId::BUTTON_LONG_PRESS:
                DEBUG("Long press succeeded");
                return AppStateId::WORKING;

            default:
                break;
        }
        return STATE_ID;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return STATE_ID;
    } 
};


class Working : public etl::fsm_state<App, Working, AppStateId::WORKING, Stop> {
public:
    etl::fsm_state_id_t on_enter_state() {
        // Temporary stub
        DEBUG("Working entered");
        blinker.once({ {0, 200}, {255, 300}, {0, 200} });
        return AppStateId::IDLE;
    }

    etl::fsm_state_id_t on_event(const Stop& event) { return AppStateId::IDLE; }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return STATE_ID;
    } 
};


class Bonding : public etl::fsm_state<App, Bonding, AppStateId::BONDING, BondOff> {
public:
    static constexpr uint32_t BONDING_PERIOD_MS = 15*1000;

    etl::fsm_state_id_t on_enter_state() {
        blinker.loop(BLINK_BONDING_LOOP);

        // Enable bonding for 30 seconds
        xTimeoutTimer = xTimerCreate("BondingTimeout", pdMS_TO_TICKS(BONDING_PERIOD_MS), pdFALSE, (void *)0,
            [](TimerHandle_t xTimer){ app.receive(BondOff()); });

        // Ideally, we should check all returned statuses, but who cares...
        if (xTimeoutTimer) xTimerStart(xTimeoutTimer, 0);

        pairing_enable();
        return STATE_ID;
    }

    void on_exit_state() {
        if (xTimeoutTimer) {
            xTimerStop(xTimeoutTimer, 0);
            xTimerDelete(xTimeoutTimer, 0);
            xTimeoutTimer = nullptr;
        }
        pairing_disable();
        blinker.off();
    }

    etl::fsm_state_id_t on_event(const BondOff& event) { return AppStateId::IDLE; }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return STATE_ID;
    }

private:
    TimerHandle_t xTimeoutTimer = nullptr;
};


Init init;
Idle idle;
Working working;
Bonding bonding;

etl::ifsm_state* stateList[AppStateId::NUMBER_OF_STATES] = {
    &init,
    &idle,
    &working,
    &bonding
};

}

void app_states_init(App& app) {
    app.set_states(stateList, AppStateId::NUMBER_OF_STATES);
}