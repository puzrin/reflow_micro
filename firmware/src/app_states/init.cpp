#include "app.hpp"
#include "logger.hpp"

namespace {

class Init : public etl::fsm_state<App, Init, DeviceState_Init> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override {
        DEBUG("State => Init");
        return DeviceState_Idle;
    }

    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }
};

Init initialize;

} // namespace

etl::ifsm_state& state_init = initialize;