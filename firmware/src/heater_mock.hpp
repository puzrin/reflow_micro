#pragma once

#include <vector>
#include <atomic>
#include <cmath>
#include "lib/adrc.hpp"
#include "components/heater_base.hpp"

class ChargerProfileMock {
public:
    ChargerProfileMock(float _V, float _I, bool _PPS = false) : V{_V}, I{_I}, PPS{_PPS} {}

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


class HeaterMock: public HeaterBase {
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

public:
    HeaterMock();

    auto get_temperature() -> float override { return temperature; }
    auto get_resistance() -> float override { return calculate_resistance(temperature); }
    auto get_max_power() -> float override { return charger.get_power(get_resistance()); }
    auto get_power() -> float override { return std::min(get_max_power(), power_setpoint.load(std::memory_order_relaxed)); }
    auto get_volts() -> float override { return std::sqrt(get_power() * get_resistance()); }
    auto get_amperes() -> float override {
        float r = get_resistance();
        return r > 0 ? std::sqrt(get_power() / r) : 0;
    }

    void start() override;
    void tick(int32_t dt_ms) override;
    auto set_sensor_calibration_point(uint32_t point_id, float temperature) -> bool override;

    // Mock-related methods
    auto calibrate_TR(float T, float R) -> HeaterMock&;
    auto calibrate_TWV(float T, float W, float V) -> HeaterMock&;
    auto scale_r_to(float new_base) -> HeaterMock&;
    auto reset() -> HeaterMock&;
    auto set_size(float x, float y, float z) -> HeaterMock&;
};
