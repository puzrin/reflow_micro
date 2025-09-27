#include <algorithm>
#include <stdexcept>
#include <cmath>

#include "components/pb2struct.hpp"
#include "heater_control_mock.hpp"
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

HeaterControlMock::HeaterControlMock()
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

auto HeaterControlMock::get_head_params_pb(std::vector<uint8_t>& pb_data) -> bool {
    pb_data = head_params.get();
    return true;
}

auto HeaterControlMock::set_head_params_pb(const std::vector<uint8_t> &pb_data) -> bool {
    head_params.set(pb_data);
    return true;
}

auto HeaterControlMock::get_head_params(HeadParams& params) -> bool {
    return pb2struct(head_params.get(), params, HeadParams_fields);
}

auto HeaterControlMock::set_head_params(const HeadParams& params) -> bool {
    std::vector<uint8_t> pb_data(HeadParams_size);
    if (!struct2pb(params, pb_data, HeadParams_fields, HeadParams_size)) { return false; }

    head_params.set(pb_data);
    return true;
}

void HeaterControlMock::validate_calibration_points() {
    std::sort(calibration_points.begin(), calibration_points.end(),
              [](const CalibrationPoint& a, const CalibrationPoint& b) { return a.T < b.T; });

    for (size_t i = 1; i < calibration_points.size(); i++) {
        if (calibration_points[i].R < calibration_points[i - 1].R) {
            throw std::runtime_error("Calibration points must have non-decreasing resistance values for increasing temperatures.");
        }
    }
}

auto HeaterControlMock::calculate_resistance(float temp) const -> float {
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

auto HeaterControlMock::calculate_heat_capacity() const -> float {
    const float material_shc = 897;     // J/kg/K for Aluminum 6061
    const float material_density = 2700; // kg/m3 for Aluminum 6061
    const float volume = size.x * size.y * size.z;
    const float mass = volume * material_density;
    return mass * material_shc;
}

auto HeaterControlMock::calculate_heat_transfer_coefficient() const -> float {
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

auto HeaterControlMock::get_room_temp() const -> float {
    auto it = std::find_if(calibration_points.begin(), calibration_points.end(),
                          [](const CalibrationPoint& p) { return p.W == 0; });
    return it != calibration_points.end() ? it->T : 25.0f;
}

auto HeaterControlMock::scale_r_to(float new_base) -> HeaterControlMock& {
    if (calibration_points.empty()) {
        throw std::runtime_error("No calibration points to scale");
    }

    float ratio = new_base / calibration_points[0].R;
    for (auto& point : calibration_points) {
        point.R *= ratio;
    }
    return *this;
}

auto HeaterControlMock::calibrate_TR(float T, float R) -> HeaterControlMock& {
    calibration_points.push_back({T, R, 0});
    temperature = T;
    validate_calibration_points();
    return *this;
}

auto HeaterControlMock::calibrate_TWV(float T, float W, float V) -> HeaterControlMock& {
    const float R = (V * V) / W;
    calibration_points.push_back({T, R, W});
    validate_calibration_points();
    return *this;
}

auto HeaterControlMock::set_size(float x, float y, float z) -> HeaterControlMock& {
    size = {x, y, z};
    return *this;
}

auto HeaterControlMock::reset() -> HeaterControlMock& {
    temperature = get_room_temp();
    return *this;
}

// Need separate thread, because can send events to app (guarded with mutexes)
void HeaterControlMock::setup() {
    static constexpr int32_t TICK_PERIOD_MS = 20;
    prev_tick_ms = get_time_ms() - TICK_PERIOD_MS;

    xTaskCreate(
        [](void* params) {
            auto* self = static_cast<HeaterControlMock*>(params);
            while (true) {
                // Work at 10x speed for convenience
                self->tick();
                vTaskDelay(pdMS_TO_TICKS(TICK_PERIOD_MS));
            }
        }, "HeaterControlMock", 1024*4, this, 4, nullptr
    );
}

void HeaterControlMock::tick() {
    // Iterate temperature
    uint32_t dt_ms = get_time_ms() - prev_tick_ms;
    static constexpr float dt_inv_multiplier = 0.001f;
    const float dt = static_cast<float>(dt_ms) * dt_inv_multiplier;

    const float curr_temp = temperature;
    const float clamped_power = get_power();

    const float heat_capacity = calculate_heat_capacity();
    const float heat_transfer_coeff = calculate_heat_transfer_coefficient();
    const float temp_change = ((clamped_power - heat_transfer_coeff * (curr_temp - get_room_temp())) * dt) / heat_capacity;

    temperature = curr_temp + temp_change;

    // Call base method with main logic
    HeaterControlBase::tick();
}
