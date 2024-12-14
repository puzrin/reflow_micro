#include <algorithm>
#include <stdexcept>
#include <cmath>
#include "heater_mock.hpp"

ChargerMock& ChargerMock::add(const Profile& profile) {
    profiles.push_back(profile);
    return *this;
}

float ChargerMock::get_power(float R) const {
    float max_power = 0;
    for (const auto& profile : profiles) {
        if (profile.is_useable(R)) {
            float power = profile.get_power(R);
            max_power = std::max(max_power, power);
        }
    }
    return max_power;
}

static float interpolate(float x, const std::vector<std::pair<float, float>>& points) {
    if (points.size() < 2) {
        throw std::runtime_error("At least two points are required for interpolation.");
    }

    if (x <= points[0].first) {
        const auto& [x1, y1] = points[0];
        const auto& [x2, y2] = points[1];
        return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1);
    }

    if (x >= points.back().first) {
        const auto& [x1, y1] = points[points.size() - 2];
        const auto& [x2, y2] = points.back();
        return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1);
    }

    for (size_t i = 1; i < points.size(); i++) {
        if (x < points[i].first) {
            const auto& [x1, y1] = points[i - 1];
            const auto& [x2, y2] = points[i];
            return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1);
        }
    }

    throw std::runtime_error("Interpolation error: x is out of bounds.");
}

static ChargerMock make_charger_140w_with_pps() {
    ChargerMock charger;
    charger.add(ChargerMock::Profile(9, 3))
           .add(ChargerMock::Profile(12, 3))
           .add(ChargerMock::Profile(15, 3))
           .add(ChargerMock::Profile(20, 5))
           .add(ChargerMock::Profile(21, 5, true))
           .add(ChargerMock::Profile(28, 5));
    return charger;
}

HeaterMock::HeaterMock()
    : size{0.08f, 0.07f, 0.0038f}
    , temperature{25.0f}
    , power_setpoint{0}
    , temperature_setpoint{25.0f}
    , temperature_control_flag{false}
{
    profiles = make_charger_140w_with_pps();

    calibrate_TR(25, 1.6f)
        .calibrate_TWV(102, 11.63f, 5.0f)
        .calibrate_TWV(146, 20.17f, 7.0f)
        .calibrate_TWV(193, 29.85f, 9.0f)
        .calibrate_TWV(220, 40.66f, 11.0f)
        .calibrate_TWV(255, 52.06f, 13.0f)
        .calibrate_TWV(286, 64.22f, 15.0f)
        .calibrate_TWV(310, 77.55f, 17.0f);
}

void HeaterMock::validate_calibration_points() {
    std::sort(calibration_points.begin(), calibration_points.end(),
              [](const CalibrationPoint& a, const CalibrationPoint& b) { return a.T < b.T; });

    for (size_t i = 1; i < calibration_points.size(); i++) {
        if (calibration_points[i].R < calibration_points[i - 1].R) {
            throw std::runtime_error("Calibration points must have non-decreasing resistance values for increasing temperatures.");
        }
    }
}

float HeaterMock::calculate_resistance(float temp) const {
    if (calibration_points.empty()) {
        throw std::runtime_error("No calibration points defined.");
    }

    if (calibration_points.size() == 1) {
        const auto& point = calibration_points[0];
        return point.R * (1 + TUNGSTEN_TC * (temp - point.T));
    }

    std::vector<std::pair<float, float>> points;
    points.reserve(calibration_points.size());
    for (const auto& p : calibration_points) {
        points.emplace_back(p.T, p.R);
    }
    return interpolate(temp, points);
}

float HeaterMock::calculate_heat_capacity() const {
    const float material_shc = 897;     // J/kg/K for Aluminum 6061
    const float material_density = 2700; // kg/m3 for Aluminum 6061
    const float volume = size.x * size.y * size.z;
    const float mass = volume * material_density;
    return mass * material_shc;
}

float HeaterMock::calculate_heat_transfer_coefficient() const {
    std::vector<CalibrationPoint> points_with_power;
    std::copy_if(calibration_points.begin(), calibration_points.end(),
                 std::back_inserter(points_with_power),
                 [](const CalibrationPoint& p) { return p.W != 0; });

    if (points_with_power.empty()) {
        const float area = size.x * size.y; // in m²
        return 40 * area; // in W/K, Default empiric value, when no calibration data is available
    }

    if (points_with_power.size() == 1) {
        const auto& point = points_with_power[0];
        return point.W / (point.T - get_room_temp());
    }

    std::vector<std::pair<float, float>> points;
    points.reserve(points_with_power.size());
    float room_t = get_room_temp();
    for (const auto& p : points_with_power) {
        points.emplace_back(p.T, p.W / (p.T - room_t));
    }
    return interpolate(temperature, points);
}

float HeaterMock::get_room_temp() const {
    auto it = std::find_if(calibration_points.begin(), calibration_points.end(),
                          [](const CalibrationPoint& p) { return p.W == 0; });
    return it != calibration_points.end() ? it->T : 25.0f;
}

float HeaterMock::get_max_power() const {
    return profiles.get_power(get_resistance());
}

HeaterMock& HeaterMock::scale_r_to(float new_base) {
    if (calibration_points.empty()) {
        throw std::runtime_error("No calibration points to scale");
    }

    float ratio = new_base / calibration_points[0].R;
    for (auto& point : calibration_points) {
        point.R *= ratio;
    }
    return *this;
}

HeaterMock& HeaterMock::calibrate_TR(float T, float R) {
    calibration_points.push_back({T, R, 0});
    temperature = T;
    validate_calibration_points();
    return *this;
}

HeaterMock& HeaterMock::calibrate_TWV(float T, float W, float V) {
    float R = (V * V) / W;
    calibration_points.push_back({T, R, W});
    validate_calibration_points();
    return *this;
}

HeaterMock& HeaterMock::set_size(float x, float y, float z) {
    size = {x, y, z};
    return *this;
}

HeaterMock& HeaterMock::reset() {
    temperature = get_room_temp();
    return *this;
}

void HeaterMock::iterate(float dt) {
    float curr_temp = temperature;
    float clamped_power = get_power();

    float heat_capacity = calculate_heat_capacity();
    float heat_transfer_coeff = calculate_heat_transfer_coefficient();
    float temp_change = ((clamped_power - heat_transfer_coeff * (curr_temp - get_room_temp())) * dt) / heat_capacity;

    temperature = curr_temp + temp_change;

    if (temperature_control_flag) {
        float power = adrc.iterate(curr_temp, temperature_setpoint, get_max_power(), dt);
        set_power(power);
    }
}

void HeaterMock::temperature_control_on() {
    adrc.reset_to(temperature);
    temperature_control_flag = true;
}

void HeaterMock::temperature_control_off() {
    temperature_control_flag = false;
    set_power(0);
    set_temperature(get_room_temp());
}
