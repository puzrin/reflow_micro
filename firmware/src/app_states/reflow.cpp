#include <etl/algorithm.h>

#include "components/profiles_config.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"
#include "reflow.hpp"


void Timeline::load(const Profile& profile) {
    profilePoints.clear();
    segmentRates_c_per_s.clear();

    profilePoints.push_back({
        0 * x_axis_multiplier,
        Constants::START_TEMPERATURE * y_axis_multiplier
    });

    for (size_t i = 0; i < profile.segments_count; ++i) {
        const auto& segment = profile.segments[i];
        profilePoints.push_back({
            profilePoints[i].time_x1000 + segment.duration * x_axis_multiplier,
            segment.target * y_axis_multiplier
        });
    }

    if (profilePoints.size() <= 1) { return; }

    for (size_t i = 1; i < profilePoints.size(); ++i) {
        const auto& p0 = profilePoints[i - 1];
        const auto& p1 = profilePoints[i];

        float delta_time = static_cast<float>(p1.time_x1000 - p0.time_x1000) / x_axis_multiplier;
        float delta_value = static_cast<float>(p1.value_x100 - p0.value_x100) / y_axis_multiplier;
        float rate_c_per_s = 0.0f;

        if (delta_time > 0.001f) {
            rate_c_per_s = delta_value / delta_time;
        } else {
            if (delta_value > 0.0f) { rate_c_per_s = 100.0f; }
            else if (delta_value < 0.0f) { rate_c_per_s = -100.0f; }
        }

        segmentRates_c_per_s.push_back(etl::clamp(rate_c_per_s, -100.0f, 100.0f));
    }
}

auto Timeline::get_max_time_x1000() const -> int32_t {
    if (profilePoints.size() <= 1) { return 0; }
    return profilePoints.back().time_x1000;
}

auto Timeline::get_target(int32_t offset_x1000) const -> float {
    if (offset_x1000 < 0) { return 0; }

    for (size_t i = 1; i < profilePoints.size(); ++i) {
        const auto& p0 = profilePoints[i - 1];
        const auto& p1 = profilePoints[i];

        if (p0.time_x1000 <= offset_x1000 && p1.time_x1000 >= offset_x1000) {
            int32_t delta_time_x1000 = p1.time_x1000 - p0.time_x1000;
            if (delta_time_x1000 <= 0) {
                return static_cast<float>(p1.value_x100) * y_axis_multiplier_inv;
            }
            int32_t scaled_y = p0.value_x100
                + (p1.value_x100 - p0.value_x100)
                    * (offset_x1000 - p0.time_x1000)
                    / delta_time_x1000;
            return static_cast<float>(scaled_y) * y_axis_multiplier_inv;
        }
    }

    return 0;
}

auto Timeline::get_rate(int32_t offset_x1000) const -> float {
    if (offset_x1000 < 0) { return 0; }

    for (size_t i = 1; i < profilePoints.size(); ++i) {
        if (profilePoints[i].time_x1000 >= offset_x1000) {
            return segmentRates_c_per_s[i - 1];
        }
    }

    return 0;
}


auto Reflow_State::on_enter_state() -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();
    APP_LOGI("State => Reflow");

    // Pick active profile, terminate on fail
    auto profile = std::make_unique<Profile>();
    if (!profiles_config.get_selected_profile(*profile)) {
        app.beepTaskTerminated();
        return DeviceActivityStatus_Idle;
    }

    // Load timeline and try to execute the task
    timeline.load(*profile);
    auto status = heater.task_start(profile->id, [this](int32_t time_ms) {
        task_iterator(time_ms);
    });
    if (!status) {
        app.beepTaskTerminated();
        return DeviceActivityStatus_Idle;
    }

    // Enable ADRC & blink about success
    heater.temperature_control_on();
    app.showReflowStart();
    app.beepTaskStarted();

    return No_State_Change;
}

auto Reflow_State::on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    event.succeeded ? app.beepTaskSucceeded() : app.beepTaskTerminated();
    return DeviceActivityStatus_Idle;
}

auto Reflow_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) {
        app.beepTaskTerminated();
        return DeviceActivityStatus_Idle;
    }
    return No_State_Change;
}

auto Reflow_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void Reflow_State::on_exit_state() {
    heater.task_stop();
}

void Reflow_State::task_iterator(int32_t time_ms) {
    auto& app = get_fsm_context();

    if (time_ms >= timeline.get_max_time_x1000()) {
        heater.task_stop();
        app.enqueue_message(AppCmd::Stop{true});
        return;
    }

    heater.set_temperature(timeline.get_target(time_ms), timeline.get_rate(time_ms));
}
