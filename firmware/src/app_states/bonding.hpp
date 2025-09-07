#include "app.hpp"
#include "proto/generated/types.pb.h"

class Bonding_State : public etl::fsm_state<App, Bonding_State, DeviceActivityStatus_Bonding,
    AppCmd::BondOff, AppCmd::Button> {
public:
    static constexpr int32_t BONDING_PERIOD_MS = 15 * 1000;

    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::BondOff& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;

private:
    TimerHandle_t xTimeoutTimer{nullptr};
};
