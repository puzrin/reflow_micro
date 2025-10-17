#pragma once

#include <etl/memory.h>
#include <etl/vector.h>

#include "app.hpp"
#include "proto/generated/types.pb.h"

class StepResponse_State : public etl::fsm_state<App, StepResponse_State, DeviceActivityStatus_StepResponse,
    AppCmd::Stop, AppCmd::Succeeded, AppCmd::Button> {
public:
    struct LogEntry {
        int16_t temperature_x10;  // Temperature * 10
        uint16_t power_x10;       // Power in watts * 10
        uint16_t time_x50;        // Time in seconds * 50 (resolution 0.02s)
    };

    static constexpr int32_t LOG_INTERVAL_MS = 500;
    static constexpr int32_t MAX_LOG_DURATION_MS = 1'000'000;  // 1000 seconds max
    static constexpr int32_t MAX_LOG_SIZE = MAX_LOG_DURATION_MS / LOG_INTERVAL_MS;  // 2000 entries
    using LOG_STORE = etl::vector<LogEntry, MAX_LOG_SIZE>;

    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Succeeded& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;

private:
    etl::unique_ptr<LOG_STORE> log_entries_{};

    void task_iterator(int32_t dt_ms, int32_t time_ms);
    size_t find_t_idx_of(float ratio);
};
