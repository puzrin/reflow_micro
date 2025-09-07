#include "app.hpp"
#include "proto/generated/types.pb.h"

class StepResponse_State : public etl::fsm_state<App, StepResponse_State, DeviceActivityStatus_StepResponse,
    AppCmd::Stop, AppCmd::Button> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;

private:
    std::vector<std::pair<float, float>> log{};

    void task_iterator(int32_t dt_ms, int32_t time_ms);
};
