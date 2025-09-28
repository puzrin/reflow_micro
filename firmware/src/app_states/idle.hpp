#include "app.hpp"
#include "proto/generated/types.pb.h"

class Idle_State : public etl::fsm_state<App, Idle_State, DeviceActivityStatus_Idle,
    AppCmd::Reflow, AppCmd::SensorBake, AppCmd::AdrcTest, AppCmd::StepResponse, AppCmd::Button, AppCmd::Stop> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Reflow& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::AdrcTest& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::StepResponse& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;
};
