#include "app.hpp"
#include "logger.hpp"

namespace {

class StepResponse : public etl::fsm_state<App, StepResponse, DeviceState_StepResponse,
    AppCmd::Stop, AppCmd::Button> {
public:
    auto on_enter_state() -> etl::fsm_state_id_t override {
        DEBUG("State => StepResponse");

        auto& app = get_fsm_context();
        auto& heater = app.heater;

        log.clear();
        log.push_back({0, heater.get_temperature()});

        auto status = app.heater.task_start(HISTORY_ID_STEP_RESPONSE, [this](int32_t dt_ms, int32_t time_ms) {
            task_iterator(dt_ms, time_ms);
        });
        if (!status) { return DeviceState_Idle; }

        heater.set_power(app.last_cmd_data);

        return No_State_Change;
    }

    void on_exit_state() override { get_fsm_context().heater.task_stop(); }

    auto on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t {
        (void)event;
        return DeviceState_Idle;
    }
    auto on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
        if (event.type == ButtonEventId::BUTTON_PRESSED_1X) { return DeviceState_Idle; }
        return No_State_Change;
    }

    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
        get_fsm_context().LogUnknownEvent(event);
        return No_State_Change;
    }

private:
    std::vector<std::pair<float, float>> log{};

    void task_iterator(int32_t dt_ms, int32_t time_ms) {
        (void)dt_ms;
        // log index = time in seconds
        if (time_ms < log.size() * 1000) { return; }

        auto& app = get_fsm_context();
        auto& heater = app.heater;
        //static constexpr float time_inv_multiplier = 1.0f / 1000;

        log.push_back({heater.get_temperature(), heater.get_power()});

        // Ignore transport delay at start, before start to analyze data
        if (log.size() <= 10) { return; }

        // Wait for temperature to stabilize (until change is less than 1 degree
        // for 10 seconds)
        if (std::abs(log.back().first - log[log.size()-10].first) > 1.0f) { return; }

        //
        // Analyze log to find response time & b0
        //

        float max_power = 0;
        for (const auto& p : log) { max_power = std::max(max_power, p.second); }

        float t_final = log.back().first;
        float t_initial = log.front().first;

        const float temperature_63 = t_initial + (t_final - t_initial) * 0.63F;
        float time_63 = 0;

        for (size_t i = 0; i < log.size(); ++i) {
            if (log[i].first >= temperature_63) {
                time_63 = static_cast<float>(i);
                break;
            }
        }

        const float b0 = (temperature_63 - t_initial) / time_63 / max_power;

        const std::string b0_str = std::to_string(b0);
        DEBUG("Temperature = {}, time = {}, b0 = {}",
            static_cast<int>(temperature_63),
            static_cast<int>(time_63),
            b0_str.c_str());

        AdrcParams adrc_params;
        heater.get_adrc_params(adrc_params);

        adrc_params.response = time_63;
        adrc_params.b0 = b0;

        heater.set_adrc_params(adrc_params);

        heater.task_stop();
        app.receive(AppCmd::Stop());
    }
};

StepResponse stepResponse;

} // namespace

etl::ifsm_state& state_step_response = stepResponse;
