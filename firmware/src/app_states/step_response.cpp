#include "step_response.hpp"
#include "heater/heater.hpp"
#include "logger.hpp"

auto StepResponse_State::on_enter_state() -> etl::fsm_state_id_t {
    APP_LOGI("State => StepResponse");

    auto& app = get_fsm_context();

    log_entries_.reset(new LOG_STORE());
    auto& log = *log_entries_;

    log.push_back({
        .temperature_x10 = static_cast<int16_t>(heater.get_temperature() * 10.0f),
        .power_x10 = static_cast<uint16_t>(heater.get_power() * 10.0f),
        .time_x50 = 0
    });

    auto status = heater.task_start(HISTORY_ID_STEP_RESPONSE, [this](int32_t time_ms) {
        task_iterator(time_ms);
    });
    if (!status) {
        log_entries_.reset();
        return DeviceActivityStatus_Idle;
    }

    heater.set_power(app.last_cmd_data);
    app.beepTaskStarted();

    return No_State_Change;
}

auto StepResponse_State::on_event(const AppCmd::Stop& event) -> etl::fsm_state_id_t {
    auto& app = get_fsm_context();

    event.succeeded ? app.beepTaskSucceeded() : app.beepTaskTerminated();
    return DeviceActivityStatus_Idle;
}

auto StepResponse_State::on_event(const AppCmd::Button& event) -> etl::fsm_state_id_t {
    if (event.type == ButtonEventId::BUTTON_PRESSED_1X) { return DeviceActivityStatus_Idle; }
    return No_State_Change;
}

auto StepResponse_State::on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t {
    get_fsm_context().LogUnknownEvent(event);
    return No_State_Change;
}

void StepResponse_State::on_exit_state() {
    heater.task_stop();
    log_entries_.reset();
}

static constexpr int32_t MAX_TRANSPORT_DELAY_MS = 10'000;  // 10 seconds max transport delay
static constexpr int32_t STABILITY_CHECK_PERIOD_MS = 30'000;  // 30 seconds stability check period
static constexpr float STABILITY_THRESHOLD_TEMP = 1.0f;  // 1 degree Celsius

void StepResponse_State::task_iterator(int32_t time_ms) {
    if (!log_entries_) { return; }
    auto& log = *log_entries_;

    // Check for abnormal temperature jitter
    const float current_temp = heater.get_temperature();
    const float last_temp = log.back().temperature_x10 * 0.1f;
    if (std::abs(current_temp - last_temp) > 5.0f) {
        APP_LOGE("Abnormal temperature jitter detected: {} -> {}",
            static_cast<int>(last_temp), static_cast<int>(current_temp));
    }

    // Check if it's time to record a new log entry
    const int32_t last_log_time_ms = (log.back().time_x50 * 1000) / 50;
    if (time_ms < last_log_time_ms + LOG_INTERVAL_MS) { return; }

    auto& app = get_fsm_context();

    // Check if max log size reached
    if (log.size() >= MAX_LOG_SIZE) {
        app.enqueue_message(AppCmd::Stop{});
        return;
    }

    // Record new log entry
    log.push_back({
        .temperature_x10 = static_cast<int16_t>(heater.get_temperature() * 10.0f),
        .power_x10 = static_cast<uint16_t>(heater.get_power() * 10.0f),
        .time_x50 = static_cast<uint16_t>(time_ms / 20)  // seconds * 50 = ms / 20
    });

    // Skip transport delay period
    if (time_ms < MAX_TRANSPORT_DELAY_MS) { return; }

    // Check stability: temperature change < 1°C over stability period
    constexpr size_t offset = STABILITY_CHECK_PERIOD_MS / LOG_INTERVAL_MS;
    if (log.size() <= offset) { return; }

    const LogEntry& old_entry = log[log.size() - offset - 1];
    if (std::abs(log.back().temperature_x10 - old_entry.temperature_x10) > STABILITY_THRESHOLD_TEMP * 10) {
        return;
    }

    //
    // Analyze log to find response time & b0
    //

    // - Heater has long tail. Early window (28/63) use is preferable over S-K (35/85)
    // - L/τ ratio is about 0.04 => predictive model will not give benefits,
    //   ADRC is enough

    // Alternate two points for shorter log
    uint32_t idx_28 = find_t_idx_of(0.28f);
    uint32_t idx_63 = find_t_idx_of(0.63f);

    float c_28 = log[idx_28].temperature_x10 * 0.1f;
    float t_28 = log[idx_28].time_x50 / 50.0f;
    float c_63 = log[idx_63].temperature_x10 * 0.1f;
    float t_63 = log[idx_63].time_x50 / 50.0f;

    float τ = 1.502069f * (t_63 - t_28);
    float L = 1.493523f * t_28 - 0.493523f * t_63;

    const std::string τ_str = std::to_string(τ);
    const std::string L_str = std::to_string(L);

    float t_max = log[find_t_idx_of(1.0f)].temperature_x10 * 0.1f;

    APP_LOGI("Step response analysis:");
    APP_LOGI("  t max = {}°C", static_cast<int>(t_max));

    APP_LOGI("  P1(28%) = {}°C, {}sec", static_cast<int>(t_28), static_cast<int>(c_28));
    APP_LOGI("  P3(63%) = {}°C, {}sec", static_cast<int>(t_63), static_cast<int>(c_63));
    APP_LOGI("  response = {}s, effective delay = {}s", τ_str.c_str(), L_str.c_str());

    // Step is 0 => constant power.
    float du = (log[idx_63].power_x10 * 0.1f - 0);

    float b0 = (c_63 - c_28) / ((0.63f - 0.28f) * τ * du);
    // Linear fit over edge points gives twice lower value
    // float b0 = (c_63 - c_28) / ((t_63 - t_28) * du);

    const std::string b0_str = std::to_string(b0);
    APP_LOGI("  b0 = {}", b0_str.c_str());

    HeadParams p;
    heater.get_head_params(p);

    p.adrc_response = τ;
    p.adrc_b0 = b0;

    heater.set_head_params(p);
    app.enqueue_message(AppCmd::Stop{true});
}

size_t StepResponse_State::find_t_idx_of(float ratio) {
    if (!log_entries_) { return 0; }
    auto& log = *log_entries_;

    int16_t t_initial_x10 = log.front().temperature_x10;

    // Find maximum temperature in log
    int16_t t_max_x10 = log[0].temperature_x10;
    size_t max_idx = 0;
    for (size_t i = 1; i < log.size(); ++i) {
        if (log[i].temperature_x10 > t_max_x10) {
            t_max_x10 = log[i].temperature_x10;
            max_idx = i;
        }
    }

    // For ratio >= 1.0, return index of maximum temperature
    if (ratio >= 1.0f) {
        return max_idx;
    }

    // For ratio 0...1, find where temperature reaches ratio of (t_max - t_initial)
    int16_t target_temp_x10 = t_initial_x10 + static_cast<int16_t>((t_max_x10 - t_initial_x10) * ratio);

    // Find first point >= target
    size_t idx = log.size() - 1;
    for (size_t i = 0; i < log.size(); ++i) {
        if (log[i].temperature_x10 >= target_temp_x10) {
            idx = i;
            break;
        }
    }

    // Check if previous point is closer
    if (idx > 0) {
        int16_t diff_curr = std::abs(log[idx].temperature_x10 - target_temp_x10);
        int16_t diff_prev = std::abs(log[idx - 1].temperature_x10 - target_temp_x10);

        if (diff_prev < diff_curr) {
            idx = idx - 1;
        }
    }

    return idx;
}
