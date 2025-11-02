#include "components/profiles_config.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"
#include "reflow.hpp"


void Timeline::load(const Profile& profile) {
    profilePoints.clear();
    profilePoints.push_back({
        0 * x_axis_multiplier,
        Constants::START_TEMPERATURE * y_axis_multiplier
    });

    for (size_t i = 0; i < profile.segments_count; ++i) {
        const auto& segment = profile.segments[i];
        profilePoints.push_back({
            profilePoints[i].x + segment.duration * x_axis_multiplier,
            segment.target * y_axis_multiplier
        });
    }
}

auto Timeline::get_max_time() const -> int32_t {
    if (profilePoints.size() <= 1) { return 0; }
    return profilePoints.back().x;
}

auto Timeline::interpolate(int32_t offset) const -> float {
    if (offset < 0) { return 0; }

    for (size_t i = 1; i < profilePoints.size(); ++i) {
        const auto& p0 = profilePoints[i - 1];
        const auto& p1 = profilePoints[i];

        if (p0.x <= offset && p1.x >= offset) {
            int32_t scaled_y = p0.y + (p1.y - p0.y) * (offset - p0.x) / (p1.x - p0.x);
            return static_cast<float>(scaled_y) * y_axis_multiplier_inv;
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

    if (time_ms >= timeline.get_max_time()) {
        heater.task_stop();
        app.enqueue_message(AppCmd::Stop{true});
        return;
    }

    heater.set_temperature(timeline.interpolate(time_ms));
}
