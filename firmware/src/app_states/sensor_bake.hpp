#pragma once

#include "app.hpp"
#include "proto/generated/types.pb.h"

class SensorBake_State : public etl::fsm_state<App, SensorBake_State, DeviceActivityStatus_SensorBake,
    AppCmd::Stop, AppCmd::Button, AppCmd::SensorBake> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::SensorBake& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;

private:
    float last_temperature = 0.0f;

    void task_iterator(int32_t time_ms);
};
