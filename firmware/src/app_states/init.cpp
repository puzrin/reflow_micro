#include "init.hpp"
#include "logger.hpp"

auto Init_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => Init");
    return DeviceState_Idle;
}

auto Init_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}
