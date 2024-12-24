#pragma once

#include <vector>
#include <atomic>
#include "lib/adrc.hpp"
#include "components/heater_base.hpp"

class ChargerMock {
public:
    class Profile {
        float V;
        float I;
        bool PPS;
    public:
        Profile(float V, float I, bool PPS = false) : V(V), I(I), PPS(PPS) {}

        bool is_useable(float R) const { return PPS ? true : (V / R) <= I; }

        float get_power(float R) const {
            if (PPS) return pow(std::min(V, I * R), 2) / R;
            return is_useable(R) ? pow(V, 2) / R : 0;
        };
    };

    ChargerMock& add(const Profile& profile);
    float get_power(float R) const;

private:
    std::vector<Profile> profiles;
};

class HeaterMock: public HeaterBase {
private:
    std::atomic<float> temperature;

    void validate_calibration_points();
    float calculate_resistance(float temp) const;
    float calculate_heat_capacity() const;
    float calculate_heat_transfer_coefficient() const;
    float get_room_temp() const;

    struct Size { float x, y, z; };
    struct CalibrationPoint { float T, R, W; };

    static constexpr float TUNGSTEN_TC = 0.0041f; // Temperature coefficient for tungsten

    Size size;
    std::vector<CalibrationPoint> calibration_points;
    ChargerMock profiles;

public:
    HeaterMock();

    float get_temperature() override { return temperature; }
    float get_resistance() override { return calculate_resistance(temperature); }
    float get_max_power() override { return profiles.get_power(get_resistance()); }
    float get_power() override { return std::min(get_max_power(), power_setpoint.load(std::memory_order_relaxed)); }
    float get_volts() override { return std::sqrt(get_power() * get_resistance()); }
    float get_amperes() override {
        float r = get_resistance();
        return r > 0 ? std::sqrt(get_power() / r) : 0;
    }

    void iterate(float dt) override;
    bool set_sensor_calibration_point(uint32_t point_id, float temperature) override;

    // Mock-related methods
    HeaterMock& calibrate_TR(float T, float R);
    HeaterMock& calibrate_TWV(float T, float W, float V);
    HeaterMock& scale_r_to(float new_base);
    HeaterMock& reset();
    HeaterMock& set_size(float x, float y, float z);
};
