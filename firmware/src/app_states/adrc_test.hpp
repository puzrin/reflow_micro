#include "app.hpp"
#include "proto/generated/types.pb.h"

class AdrcTest_State : public etl::fsm_state<App, AdrcTest_State, DeviceState_AdrcTest,
    AppCmd::Stop, AppCmd::Button, AppCmd::AdrcTest> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;
};
