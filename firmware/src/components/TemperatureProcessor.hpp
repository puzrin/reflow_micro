#pragma once

#include <stdint.h>
#include "lib/pt100.hpp"

class TemperatureProcessor {
public:
    enum class SensorType {
        RTD, // PT100
        TCR  // Based on copper/tungsten heater wire resistance change
    };

    // Defaults to use when lack of calibration points data
    static constexpr uint32_t TCR_R_DEFAULT = 4000; // in mohms
    static constexpr int32_t TCR_T_REF_DEFAULT_X10 = 25 * 10; // in Celsius * 10
    static constexpr float TCR_COEFF_DEFAULT = 0.00393f / 10.0f; // Copper TCR, scaled for 0.1°C units (original: 0.00393 [1/°C])

    TemperatureProcessor() {
        rebuild();
    }

    void set_sensor_type(SensorType type) {
        sensor_type = type;
        rebuild();
    }
    void set_cal_points(float at_0, float value_0, float at_1, float value_1) {
        p0_at = at_0; p0_value = value_0;
        p1_at = at_1; p1_value = value_1;
        rebuild();
    }

    int32_t get_temperature_x10(uint32_t at) {
        if (sensor_type == SensorType::RTD) { return get_rtd_temperature_x10(at); }
        else { return get_tcr_temperature_x10(at); }
    }

private:
    SensorType sensor_type{SensorType::RTD};
    // Calibration points
    float p0_at{0.0f};
    float p0_value{0.0f};
    float p1_at{0.0f};
    float p1_value{0.0f};

    // Coefficients for temperature calculation
    // RTD calibration: R_corrected = rtd_gain * R_raw + rtd_offset
    int32_t rtd_gain_q16{};   // Q16 fixed-point: gain = (R_expected1 - R_expected0) / (R_raw1 - R_raw0)
    int32_t rtd_offset{};     // in milliohms

    // TCR calibration: T_x10 = tcr_t_ref_x10 + (R - tcr_r_base) * inv_gain
    uint32_t tcr_r_base{};       // in milliohms
    int32_t tcr_t_ref_x10{};     // temperature x10
    int32_t tcr_inv_gain_q16{};  // Q16 fixed-point: inv_gain = dT_x10/dR


    uint8_t cal_points_count() const {
        if (p0_at <= 0) { return 0; }
        if (p1_at <= 0) { return 1; }
        // Division by zero protection: if calibration points are identical, use only first point
        if (p0_value == p1_value || p0_at == p1_at) { return 1; }
        return 2;
    }

    void prepare_rtd_coeffs() {
        float R_raw0, R_raw1;
        float R_expected0, R_expected1;

        switch(cal_points_count()) {
            case 0: // No calibration points
                rtd_gain_q16 = 1 << 16;  // 1.0 in Q16
                rtd_offset = 0;
                break;

            case 1: // One calibration point
                R_raw0 = 560.0f * p0_value / (2500.0f - p0_value) * 1000.0f;  // milliohms
                R_expected0 = PT100::x10_t2r(static_cast<int32_t>(p0_at * 10));

                rtd_gain_q16 = 1 << 16;  // 1.0 in Q16
                rtd_offset = static_cast<int32_t>(R_expected0 - R_raw0);
                break;

            case 2: // Two calibration points
                R_raw0 = 560.0f * p0_value / (2500.0f - p0_value) * 1000.0f;  // milliohms
                R_raw1 = 560.0f * p1_value / (2500.0f - p1_value) * 1000.0f;  // milliohms
                R_expected0 = PT100::x10_t2r(static_cast<int32_t>(p0_at * 10));
                R_expected1 = PT100::x10_t2r(static_cast<int32_t>(p1_at * 10));

                // gain = (R_expected1 - R_expected0) / (R_raw1 - R_raw0)
                // Division by zero is protected by cal_points_count()
                float gain = (R_expected1 - R_expected0) / (R_raw1 - R_raw0);
                rtd_gain_q16 = static_cast<int32_t>(gain * 65536.0f);

                // offset = R_expected0 - gain * R_raw0
                rtd_offset = static_cast<int32_t>(R_expected0 - gain * R_raw0);
                break;
        }
    }

    int32_t get_rtd_temperature_x10(uint32_t mV) const {
        // Calculate R_raw in milliohms: R = 560Ω * mV / (2500mV - mV) * 1000
        uint32_t R_raw = (560 * mV * 1000) / (2500 - mV);

        // Apply calibration: R_corrected = rtd_gain * R_raw + rtd_offset
        int64_t R_corrected = (static_cast<int64_t>(rtd_gain_q16) * static_cast<int64_t>(R_raw)) >> 16;
        R_corrected += rtd_offset;

        // Sanity check: shouldn't happen in normal operation (would mean catastrophic HW failure
        // or calibration done with completely different equipment)
        if (R_corrected < 0) R_corrected = 0;

        // Convert resistance to temperature using lookup table
        int32_t T_x10 = PT100::r2t_x10(static_cast<uint32_t>(R_corrected));
        return T_x10;
    }

    void prepare_tcr_coeffs() {
        float gain;
        switch(cal_points_count()) {
            case 0: // No calibration points
                tcr_r_base = TCR_R_DEFAULT;
                tcr_t_ref_x10 = TCR_T_REF_DEFAULT_X10;
                // gain = dR/dT_x10 = R_base * TCR_coeff
                gain = TCR_R_DEFAULT * TCR_COEFF_DEFAULT;
                tcr_inv_gain_q16 = static_cast<int32_t>((1.0f / gain) * 65536.0f);
                break;

            case 1: // One calibration point
                tcr_r_base = static_cast<uint32_t>(p0_value);
                tcr_t_ref_x10 = static_cast<int32_t>(p0_at * 10);
                // gain = dR/dT_x10 = R_base * TCR_coeff
                gain = p0_value * TCR_COEFF_DEFAULT;
                tcr_inv_gain_q16 = static_cast<int32_t>((1.0f / gain) * 65536.0f);
                break;

            case 2: // Two calibration points
                tcr_r_base = static_cast<uint32_t>(p0_value);
                tcr_t_ref_x10 = static_cast<int32_t>(p0_at * 10);
                // gain = dR/dT_x10 = (R1 - R0) / (T1_x10 - T0_x10)
                // Division by zero is protected by cal_points_count()
                gain = (p1_value - p0_value) / ((p1_at - p0_at) * 10.0f);
                tcr_inv_gain_q16 = static_cast<int32_t>((1.0f / gain) * 65536.0f);
                break;
        }
    }

    int32_t get_tcr_temperature_x10(uint32_t mohms) const {
        // T_x10 = T_ref_x10 + (R - R_base) * inv_gain
        // where inv_gain = dT_x10/dR
        int32_t delta_R = static_cast<int32_t>(mohms - tcr_r_base);
        int64_t delta_T_x10 = (static_cast<int64_t>(delta_R) * static_cast<int64_t>(tcr_inv_gain_q16)) >> 16;

        int32_t T_x10 = tcr_t_ref_x10 + static_cast<int32_t>(delta_T_x10);
        return T_x10;
    }

    void rebuild() {
        if (sensor_type == SensorType::RTD) { prepare_rtd_coeffs(); }
        else { prepare_tcr_coeffs(); }
    }
};
