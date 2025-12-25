#pragma once

#include <vector>

#include "app.hpp"
#include "components/time.hpp"
#include "heater_control_base.hpp"
#include "power.hpp"

class HeaterControl: public HeaterControlBase {
public:
    void setup() override;
    void tick() override;
    uint32_t get_time_ms() const override { return Time::now(); }

    void set_power(float power_w) override;
    auto task_start(int32_t task_id, HeaterTaskIteratorFn task_iterator = nullptr) -> bool;
    void task_stop();

    bool get_head_params_pb(std::vector<uint8_t>& pb_data) override;
    bool set_head_params_pb(const std::vector<uint8_t>& pb_data) override;
    bool get_head_params(HeadParams& params) override;
    bool set_head_params(const HeadParams& params) override;
    bool set_calibration_point_0(float temperature) override;
    bool set_calibration_point_1(float temperature) override;

    auto get_health_status() -> DeviceHealthStatus override;
    auto get_activity_status() -> DeviceActivityStatus override;
    auto get_power_status() -> PowerStatus override;
    auto get_head_status() -> HeadStatus override;

    auto get_temperature() -> float override;
    auto get_resistance() -> float override;
    auto get_max_power() -> float override;
    auto get_power() -> float override;
    auto get_volts() -> float override;
    auto get_amperes() -> float override;
    auto get_duty_cycle() -> float override;

private:
    void update_fan_speed();
    void update_temperature_indicator();
};
