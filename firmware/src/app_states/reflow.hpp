#pragma once

#include <etl/vector.h>

#include "app.hpp"
#include "proto/generated/types.pb.h"

class Timeline {
private:
    struct TimelinePoint { int32_t time_x1000; int32_t value_x100; };
    static constexpr size_t MAX_PROFILE_POINTS = Constants::MAX_REFLOW_SEGMENTS + 1;
    etl::vector<TimelinePoint, MAX_PROFILE_POINTS> profilePoints{};
    etl::vector<float, Constants::MAX_REFLOW_SEGMENTS> segmentRates_c_per_s{};

    // Use integer math for speed
    // Time is in milliseconds, temperature is in 1/100 degrees
    static constexpr int32_t x_axis_multiplier = 1000;
    static constexpr int32_t y_axis_multiplier = 100;
    // Inverse multiplier for division
    static constexpr float y_axis_multiplier_inv = 1.0F / y_axis_multiplier;

public:
    void load(const Profile& profile);
    auto get_max_time_x1000() const -> int32_t;
    auto get_target(int32_t offset_x1000) const -> float;
    auto get_rate(int32_t offset_x1000) const -> float;
};


class Reflow_State : public etl::fsm_state<App, Reflow_State, DeviceActivityStatus_Reflow,
    AppCmd::Stop, AppCmd::Button> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override;

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t;
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t;
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t;

    void on_exit_state() override;

private:
    Timeline timeline{};

    void task_iterator(int32_t time_ms);
};
