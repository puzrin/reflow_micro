#pragma once

#include <stdint.h>
#include <etl/vector.h>

template<size_t MAX_POINTS = 100>
class AdcInterpolator {
public:
    struct CalibPoint {
        uint16_t raw;
        uint16_t mV;
    };

    etl::vector<CalibPoint, MAX_POINTS> points;

    uint32_t to_uv(uint32_t adc_raw_scaled, uint32_t scale) const {
        if (points.empty() || scale == 0) {
            return 0;
        }

        // adc_raw_scaled is sum, scale is count
        // Handle edges with proper scaling to ensure search will find interval
        uint64_t first_scaled = (uint64_t)points.front().raw * scale;
        uint64_t last_scaled = (uint64_t)points.back().raw * scale;

        if (adc_raw_scaled <= first_scaled) {
            return (uint32_t)points.front().mV * 1000;
        }
        if (adc_raw_scaled >= last_scaled) {
            return (uint32_t)points.back().mV * 1000;
        }

        // After edge checks, adc_raw_scaled is guaranteed to be inside table range
        // Divide to get average for search (fractional part is lost but < 1 LSB)
        uint32_t avg_raw = adc_raw_scaled / scale;

        // Linear search to find nearest point to the left: points[i].raw <= avg_raw
        // First point is always left (guaranteed by edge checks)
        // Last point is always right (guaranteed by edge checks)
        // Search from second to second-to-last point
        size_t left = 0;
        for (size_t i = 1; i < points.size() - 1; i++) {
            if (points[i].raw <= avg_raw) {
                left = i;
                continue;
            }
            break;
        }
        size_t right = left + 1;

        uint64_t left_scaled = (uint64_t)points[left].raw * scale;
        uint64_t right_scaled = (uint64_t)points[right].raw * scale;

        uint64_t numerator = adc_raw_scaled - left_scaled;
        uint64_t denominator = right_scaled - left_scaled;

        uint32_t mV_left = points[left].mV;
        uint32_t mV_right = points[right].mV;

        // Calculate interpolated uV using 64-bit arithmetic to preserve fractional mV
        // uV_interp = (mV_left + (mV_right - mV_left) * numerator / denominator) * 1000
        // Rearranged to: uV_interp = (mV_left * 1000 * denominator + (mV_right - mV_left) * 1000 * numerator) / denominator
        int32_t mV_delta = (int32_t)mV_right - (int32_t)mV_left;
        int64_t uV_interp_scaled = (int64_t)mV_left * 1000 * denominator + (int64_t)mV_delta * 1000 * numerator;
        uint32_t uV_interp = (uint32_t)(uV_interp_scaled / denominator);

        return uV_interp;
    }
};
