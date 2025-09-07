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
    APP_LOGI("State => Reflow");

    auto& app = get_fsm_context();

    // Pick active profile, terminate on fail
    auto profile = std::make_unique<Profile>();
    if (!profiles_config.get_selected_profile(*profile)) { return DeviceState_Idle; }

    // Load timeline and try to execute the task
    timeline.load(*profile);
    auto status = heater.task_start(profile->id, [this](int32_t dt_ms, int32_t time_ms) {
        task_iterator(dt_ms, time_ms);
    });
    if (!status) { return DeviceState_Idle; }

    // Enable ADRC & blink about success
    heater.temperature_control_on();
    app.showReflowStart();
    app.beepReflowStarted();

    return No_State_Change;
}

auto Reflow_State::on_event(const AppCmd::Stop&) -> etl::fsm_state_id_t {
    get_fsm_context().beepReflowTerminated();
    return DeviceState_Idle;
}

auto Reflow_State::on_event(const AppCmd::Succeeded&) -> etl::fsm_state_id_t {
    get_fsm_context().beepReflowComplete();
    return DeviceState_Idle;
}

auto Reflow_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) {
        get_fsm_context().beepReflowTerminated();
        return DeviceState_Idle;
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

void Reflow_State::task_iterator(int32_t /*dt_ms*/, int32_t time_ms) {
    auto& app = get_fsm_context();

    //if (time_ms % 1000 == 0) {
    //    APP_LOGI("Reflow: time={}ms, temp={}", time_ms, timeline.interpolate(time_ms));
    //}

    if (time_ms >= timeline.get_max_time()) {
        heater.task_stop();
        app.enqueue_message(AppCmd::Succeeded{});
        return;
    }

    heater.set_temperature(timeline.interpolate(time_ms));
}
