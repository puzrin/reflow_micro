#pragma once

#include <vector>
#include <etl/atomic.h>
#include <functional>
#include "components/prefs.hpp"
#include "components/history.hpp"
#include "lib/adrc.hpp"
#include "proto/generated/types.pb.h"

using HeaterTaskIteratorFn = std::function<void(uint32_t, uint32_t)>;

class HeaterControlBase {
public:
    virtual bool get_head_params_pb(std::vector<uint8_t>& pb_data) = 0;
    virtual bool set_head_params_pb(const std::vector<uint8_t>& pb_data) = 0;
    virtual bool get_head_params(HeadParams& params) = 0;
    virtual bool set_head_params(const HeadParams& params) = 0;
    virtual bool set_calibration_point_0(float temperature) = 0;
    virtual bool set_calibration_point_1(float temperature) = 0;

    void get_history(int32_t client_history_version, float from, std::vector<uint8_t>& pb_data);

    virtual void setup() = 0;
    virtual auto load_all_params() -> bool;

    virtual auto get_health_status() -> DeviceHealthStatus = 0;
    virtual auto get_activity_status() -> DeviceActivityStatus = 0;
    virtual auto get_power_status() -> PowerStatus = 0;
    virtual auto get_head_status() -> HeadStatus = 0;

    virtual auto get_temperature() -> float = 0;
    virtual auto get_resistance() -> float = 0;
    virtual auto get_max_power() -> float = 0;
    virtual auto get_power() -> float = 0;
    virtual auto get_volts() -> float = 0;
    virtual auto get_amperes() -> float = 0;
    virtual auto get_duty_cycle() -> float = 0;

    virtual uint32_t get_time_ms() const = 0;
    virtual void set_power(float power) = 0;
    virtual void set_temperature(float temp) { temperature_setpoint = temp; }
    virtual void temperature_control_on();
    virtual void temperature_control_off();

    virtual void tick();

    // "task" machinery, by default record history.

    auto task_start(int32_t task_id, HeaterTaskIteratorFn task_iterator = nullptr) -> bool;
    void task_stop();

protected:
    ADRC adrc{};
    etl::atomic<bool> temperature_control_enabled{false};
    etl::atomic<float> temperature_setpoint{0};
    etl::atomic<bool> is_task_active{false};
    int32_t prev_tick_ms{0};

private:
    HeaterTaskIteratorFn task_iterator{nullptr};
    int32_t task_start_ts{0};
    History history{};
    int32_t history_version{0};
    int32_t history_task_id{0};
    int32_t history_last_recorded_ts{0}; // in seconds
    static constexpr int32_t history_y_multiplier = 100;
    static constexpr float history_y_multiplier_inv = 1.0F / history_y_multiplier;
};
