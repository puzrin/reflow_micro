#include "app.hpp"
#include "logger.hpp"

namespace {

class Timeline {
private:
    struct TimelinePoint { int32_t x; int32_t y; };
    std::vector<TimelinePoint> profilePoints;

    // Use integer math for speed
    // Time is in milliseconds, temperature is in 1/256 degrees
    static constexpr int32_t x_axis_multiplier = 1000;
    static constexpr int32_t y_axis_multiplier = 256;
    // Inverse multiplier for division
    static constexpr float y_axis_multiplier_inv = 1.0f / y_axis_multiplier;

public:
    void load(const Profile& profile) {
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

    int32_t get_max_time() const {
        if (profilePoints.size() <= 1) return 0;
        return profilePoints.back().x;
    }

    float interpolate(int32_t offset) const {
        if (offset < 0) return 0;

        for (size_t i = 1; i < profilePoints.size(); ++i) {
            const auto& p0 = profilePoints[i - 1];
            const auto& p1 = profilePoints[i];

            if (p0.x <= offset && p1.x >= offset) {
                int32_t scaled_y = p0.y + (p1.y - p0.y) / (p1.x - p0.x) * (offset - p0.x);
                return scaled_y * y_axis_multiplier_inv;
            }
        }

        return 0;
    }
};


class Reflow : public etl::fsm_state<App, Reflow, DeviceState_Reflow,
    AppCmd::Stop, AppCmd::Button> {
public:
    etl::fsm_state_id_t on_enter_state() {
        DEBUG("State => Reflow");

        auto& app = get_fsm_context();

        // Pick active profile, terminate on fail
        auto profile = std::make_unique<Profile>();
        if (!app.profilesConfig.get_selected_profile(*profile)) return DeviceState_Idle;

        // Load timeline and try to execute the task
        timeline.load(*profile);
        auto status = app.heater.task_start(profile->id, [this](int32_t dt_ms, int32_t time_ms) {
            task_iterator(dt_ms, time_ms);
        });
        if (!status) return DeviceState_Idle;

        // Enable ADRC & blink about success
        app.heater.temperature_control_on();
        app.blinker.once({ {0, 200}, {255, 300}, {0, 200} });

        return No_State_Change;
    }

    void on_exit_state() {
        get_fsm_context().heater.task_stop();
    }

    etl::fsm_state_id_t on_event(const AppCmd::Stop& event) { return DeviceState_Idle; }
    etl::fsm_state_id_t on_event(const AppCmd::Button& event) {
        if (event.type == ButtonEventId::BUTTON_PRESSED_1X) return DeviceState_Idle;
        return No_State_Change;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& event) {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }

private:
    Timeline timeline;
    void task_iterator(int32_t dt_ms, int32_t time_ms) {
        auto& app = get_fsm_context();

        if (time_ms >= timeline.get_max_time()) {
            app.heater.task_stop();
            app.safe_receive(AppCmd::Stop());
            return;
        }

        app.heater.set_temperature(timeline.interpolate(time_ms));
    }
};

Reflow reflow;

} // namespace

etl::ifsm_state& state_reflow = reflow;
