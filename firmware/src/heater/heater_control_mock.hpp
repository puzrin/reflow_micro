#pragma once

#include <vector>
#include <atomic>
#include <cmath>

#include "app.hpp"
#include "components/prefs.hpp"
#include "components/time.hpp"
#include "lib/adrc.hpp"
#include "heater_control_base.hpp"
#include "proto/generated/defaults.hpp"

class ChargerProfileMock {
public:
    ChargerProfileMock(float V, float I, bool PPS = false) : V{V}, I{I}, PPS{PPS} {}

    auto is_useable(float R) const -> bool { return PPS ? true : (V / R) <= I; }

    auto get_power(float R) const -> float {
        if (PPS) { return powf(std::min(V, I * R), 2) / R; }
        return is_useable(R) ? powf(V, 2) / R : 0;
    };
private:
    float V;
    float I;
    bool PPS;
};

class ChargerMock {
public:
    ChargerMock() : profiles{} {};
    auto add(const ChargerProfileMock& profile) -> ChargerMock&;
    auto get_power(float R) const -> float;

private:
    std::vector<ChargerProfileMock> profiles;
};


class HeaterControlMock: public HeaterControlBase {
private:
    std::atomic<float> temperature;

    void validate_calibration_points();
    auto calculate_resistance(float temp) const -> float;
    auto calculate_heat_capacity() const -> float;
    auto calculate_heat_transfer_coefficient() const -> float;
    auto get_room_temp() const -> float;

    struct Size { float x, y, z; };
    struct CalibrationPoint { float T, R, W; };

    static constexpr float TUNGSTEN_TC = 0.0041F; // Temperature coefficient for tungsten

    Size size;
    std::vector<CalibrationPoint> calibration_points;
    ChargerMock charger;

    AsyncPreference<std::vector<uint8_t>> head_params{
        PrefsWriter::getInstance(),
        AsyncPreferenceKV::getInstance(),
        PREFS_NAMESPACE,
        "head",
        std::vector<uint8_t>{std::begin(DEFAULT_HEAD_PARAMS_PB), std::end(DEFAULT_HEAD_PARAMS_PB)}
    };

    std::atomic<float> power_setpoint{0};

public:
    HeaterControlMock();

    bool get_head_params_pb(std::vector<uint8_t>& pb_data) override;
    bool set_head_params_pb(const std::vector<uint8_t>& pb_data) override;
    bool get_head_params(HeadParams& params) override;
    bool set_head_params(const HeadParams& params) override;

    auto get_health_status() -> DeviceHealthStatus override { return DeviceHealthStatus_DevOK; }
    auto get_activity_status() -> DeviceActivityStatus override {
        return static_cast<DeviceActivityStatus>(application.get_state_id());
    }
    auto get_power_status() -> PowerStatus override { return PowerStatus_PwrOK; }
    auto get_head_status() -> HeadStatus override { return HeadStatus_HeadConnected; }

    auto get_temperature() -> float override { return temperature; }
    auto get_resistance() -> float override { return calculate_resistance(temperature); }
    auto get_max_power() -> float override { return charger.get_power(get_resistance()); }
    auto get_power() -> float override { return std::min(get_max_power(), power_setpoint.load(std::memory_order_relaxed)); }
    auto get_volts() -> float override { return std::sqrt(get_power() * get_resistance()); }
    auto get_amperes() -> float override {
        const float r = get_resistance();
        return r > 0 ? std::sqrt(get_power() / r) : 0;
    }
    auto get_duty_cycle() -> float override { return 1.0F; }
    uint32_t get_time_ms() const override {
        // Increase simulation speed 10x
        return Time::now() * 10;
    }
    void set_power(float power) override {
        power_setpoint = (power < 0 ? 0 : power);
    }
    void setup() override;
    void tick() override;

    // Mock-related methods
    auto calibrate_TR(float T, float R) -> HeaterControlMock&;
    auto calibrate_TWV(float T, float W, float V) -> HeaterControlMock&;
    auto scale_r_to(float new_base) -> HeaterControlMock&;
    auto reset() -> HeaterControlMock&;
    auto set_size(float x, float y, float z) -> HeaterControlMock&;
};
