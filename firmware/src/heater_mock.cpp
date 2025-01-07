#include <algorithm>
#include <stdexcept>
#include <cmath>
#include "heater_mock.hpp"
#include "proto/generated/defaults.hpp"

auto ChargerMock::add(const ChargerProfileMock& profile) -> ChargerMock& {
    profiles.push_back(profile);
    return *this;
}

auto ChargerMock::get_power(float R) const -> float {
    float max_power = 0;
    for (const auto& profile : profiles) {
        if (profile.is_useable(R)) {
            float power = profile.get_power(R);
            max_power = std::max(max_power, power);
        }
    }
    return max_power;
}

static auto interpolate(float x, const std::vector<std::pair<float, float>>& points) -> float {
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

    // This will never be reached
    throw std::runtime_error("Interpolation error: x is out of bounds.");
}

static auto make_charger_140w_with_pps() -> ChargerMock {
    ChargerMock charger;
    charger.add(ChargerProfileMock(9, 3))
           .add(ChargerProfileMock(12, 3))
           .add(ChargerProfileMock(15, 3))
           .add(ChargerProfileMock(20, 5))
           .add(ChargerProfileMock(21, 5, true))
           .add(ChargerProfileMock(28, 5));
    return charger;
}

HeaterMock::HeaterMock()
    : size{0.08F, 0.07F, 0.0038F}
    , charger{make_charger_140w_with_pps()}
{
    calibrate_TR(25, 1.6F)
        .calibrate_TWV(102, 11.63F, 5.0F)
        .calibrate_TWV(146, 20.17F, 7.0F)
        .calibrate_TWV(193, 29.85F, 9.0F)
        .calibrate_TWV(220, 40.66F, 11.0F)
        .calibrate_TWV(255, 52.06F, 13.0F)
        .calibrate_TWV(286, 64.22F, 15.0F)
        .calibrate_TWV(310, 77.55F, 17.0F)
        // Transform resistance/size from calibrated heater to actual one,
        // until we have real data
        .scale_r_to(4.0F)
        .set_size(0.08F, 0.07F, 0.0028F);

    temperature = get_room_temp();
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

auto HeaterMock::calculate_resistance(float temp) const -> float {
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

auto HeaterMock::calculate_heat_capacity() const -> float {
    const float material_shc = 897;     // J/kg/K for Aluminum 6061
    const float material_density = 2700; // kg/m3 for Aluminum 6061
    const float volume = size.x * size.y * size.z;
    const float mass = volume * material_density;
    return mass * material_shc;
}

auto HeaterMock::calculate_heat_transfer_coefficient() const -> float {
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
    const float room_t = get_room_temp();
    for (const auto& p : points_with_power) {
        points.emplace_back(p.T, p.W / (p.T - room_t));
    }
    return interpolate(temperature, points);
}

auto HeaterMock::get_room_temp() const -> float {
    auto it = std::find_if(calibration_points.begin(), calibration_points.end(),
                          [](const CalibrationPoint& p) { return p.W == 0; });
    return it != calibration_points.end() ? it->T : 25.0f;
}

auto HeaterMock::scale_r_to(float new_base) -> HeaterMock& {
    if (calibration_points.empty()) {
        throw std::runtime_error("No calibration points to scale");
    }

    float ratio = new_base / calibration_points[0].R;
    for (auto& point : calibration_points) {
        point.R *= ratio;
    }
    return *this;
}

auto HeaterMock::calibrate_TR(float T, float R) -> HeaterMock& {
    calibration_points.push_back({T, R, 0});
    temperature = T;
    validate_calibration_points();
    return *this;
}

auto HeaterMock::calibrate_TWV(float T, float W, float V) -> HeaterMock& {
    const float R = (V * V) / W;
    calibration_points.push_back({T, R, W});
    validate_calibration_points();
    return *this;
}

auto HeaterMock::set_size(float x, float y, float z) -> HeaterMock& {
    size = {x, y, z};
    return *this;
}

auto HeaterMock::reset() -> HeaterMock& {
    temperature = get_room_temp();
    return *this;
}

// Need separate thread, because can send events to app (guarded with mutexes)
void HeaterMock::start() {
    // Work at 10x speed for convenience
    xTaskCreate(
        [](void* params) {
            auto* self = static_cast<HeaterMock*>(params);
            while (true) {
                self->tick(100);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }, "heater mock", 1024*4, this, 4, nullptr
    );
}

void HeaterMock::tick(int32_t dt_ms) {
    // Iterate temperature
    static constexpr float dt_inv_multiplier = 1.0F / 1000.0F;
    const float dt = static_cast<float>(dt_ms) * dt_inv_multiplier;

    const float curr_temp = temperature;
    const float clamped_power = get_power();

    const float heat_capacity = calculate_heat_capacity();
    const float heat_transfer_coeff = calculate_heat_transfer_coefficient();
    const float temp_change = ((clamped_power - heat_transfer_coeff * (curr_temp - get_room_temp())) * dt) / heat_capacity;

    temperature = curr_temp + temp_change;

    // Call base method with main logic
    HeaterBase::tick(dt_ms);
}

auto HeaterMock::set_sensor_calibration_point(uint32_t point_id, float temperature) -> bool {
    if (!is_hotplate_connected()) { return false; }

    SensorParams sensor_params;
    if (!get_sensor_params(sensor_params)) { return false; }

    if (point_id == 0) {
        sensor_params.p0_temperature = temperature;
    } else if (point_id == 1) {
        sensor_params.p1_temperature = temperature;
    } else {
        return false;
    }

    return set_sensor_params(sensor_params);
}
