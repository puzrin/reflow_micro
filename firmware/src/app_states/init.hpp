#include "app.hpp"
#include "proto/generated/types.pb.h"

class Init_State : public etl::fsm_state<App, Init_State, DeviceState_Init> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;
};
