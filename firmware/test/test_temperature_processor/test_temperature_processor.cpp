#include <gtest/gtest.h>
#include "components/TemperatureProcessor.hpp"
#include "lib/pt100.hpp"

// Voltage thresholds from Head class
static constexpr uint32_t SENSOR_SHORTED_LEVEL_MV = 150;
static constexpr uint32_t SENSOR_FLOATING_LEVEL_MV = 800;

// Helper: Convert temperature to expected ADC millivolts for PT100
// Inverse of above: mV = 2500 * R / (R + 560 * 1000)
static uint32_t temp_to_mv(int32_t temp_x10) {
    uint32_t R = PT100::x10_t2r(temp_x10);
    return 2500 * R / (R + 560 * 1000);
}

//=============================================================================
// RTD Mode Tests
//=============================================================================

TEST(TemperatureProcessorTest, RTD_NoCalibration) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);
    // No calibration points set

    // Test at room temperature ~25°C
    uint32_t mv_25c = temp_to_mv(25 * 10);
    int32_t result = proc.get_temperature_x10(mv_25c);
    EXPECT_NEAR(result, 25 * 10, 10); // ±1°C tolerance

    // Test at 100°C
    uint32_t mv_100c = temp_to_mv(100 * 10);
    result = proc.get_temperature_x10(mv_100c);
    EXPECT_NEAR(result, 100 * 10, 10);

    // Test at 200°C
    uint32_t mv_200c = temp_to_mv(200 * 10);
    result = proc.get_temperature_x10(mv_200c);
    EXPECT_NEAR(result, 200 * 10, 20);
}

TEST(TemperatureProcessorTest, RTD_OnePointCalibration_Offset) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);

    // Simulate ADC with -5mV offset: actual ADC reads 5mV lower than expected
    uint32_t mv_100c_ideal = temp_to_mv(100 * 10);
    uint32_t mv_100c_actual = mv_100c_ideal - 5;

    // BEFORE calibration: reading is wrong due to offset
    int32_t result_before = proc.get_temperature_x10(mv_100c_actual);
    int32_t error_before = abs(result_before - 100 * 10);
    EXPECT_GT(error_before, 10); // Should have noticeable error

    // AFTER calibration at 100°C
    proc.set_cal_points(100.0f, static_cast<float>(mv_100c_actual), 0.0f, 0.0f);

    int32_t result_after = proc.get_temperature_x10(mv_100c_actual);
    EXPECT_NEAR(result_after, 100 * 10, 5); // Now reads correctly

    // Check correction improved the reading (works for both positive and negative offset)
    int32_t error_after = abs(result_after - 100 * 10);
    EXPECT_GT(error_before, error_after); // Error decreased

    // Test at different temperature with same offset - should also improve
    uint32_t mv_200c_actual = temp_to_mv(200 * 10) - 5;
    result_after = proc.get_temperature_x10(mv_200c_actual);
    EXPECT_NEAR(result_after, 200 * 10, 10);
}

TEST(TemperatureProcessorTest, RTD_TwoPointCalibration_Linear) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);

    // Simulate ADC with 2% gain error and 3mV offset
    auto simulate_adc = [](uint32_t ideal_mv) -> uint32_t {
        return static_cast<uint32_t>(ideal_mv * 1.02f + 3.0f);
    };

    uint32_t mv_50c = simulate_adc(temp_to_mv(50 * 10));
    uint32_t mv_150c = simulate_adc(temp_to_mv(150 * 10));
    uint32_t mv_250c = simulate_adc(temp_to_mv(250 * 10));

    // BEFORE calibration: readings are wrong due to gain + offset error
    int32_t result_50_before = proc.get_temperature_x10(mv_50c);
    int32_t result_150_before = proc.get_temperature_x10(mv_150c);
    int32_t result_250_before = proc.get_temperature_x10(mv_250c);

    EXPECT_GT(result_50_before, 50 * 10 + 10);   // Should read higher
    EXPECT_GT(result_150_before, 150 * 10 + 20); // Error grows with temp
    EXPECT_GT(result_250_before, 250 * 10 + 30);

    // AFTER 2-point calibration at 50°C and 250°C
    proc.set_cal_points(50.0f, static_cast<float>(mv_50c),
                       250.0f, static_cast<float>(mv_250c));

    // Test at calibration points - should be corrected
    int32_t result_50_after = proc.get_temperature_x10(mv_50c);
    EXPECT_NEAR(result_50_after, 50 * 10, 5);

    int32_t result_250_after = proc.get_temperature_x10(mv_250c);
    EXPECT_NEAR(result_250_after, 250 * 10, 10);

    // Test at intermediate point - should also be corrected
    int32_t result_150_after = proc.get_temperature_x10(mv_150c);
    EXPECT_NEAR(result_150_after, 150 * 10, 10);

    // Verify correction improved readings
    EXPECT_LT(result_50_after, result_50_before);
    EXPECT_LT(result_150_after, result_150_before);
    EXPECT_LT(result_250_after, result_250_before);
}

TEST(TemperatureProcessorTest, RTD_RealisticRange) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);

    // Test across realistic soldering temperature range
    // Max safe temp ~400°C (800mV limit with 560Ω divider and ADC_ATTEN_DB_0)
    // 350°C → 738mV, 400°C → 780mV, both within 800mV threshold
    for (int32_t temp = 0; temp <= 350; temp += 50) {
        uint32_t mv = temp_to_mv(temp * 10);

        // Check voltage is in valid sensor range
        EXPECT_GT(mv, SENSOR_SHORTED_LEVEL_MV);
        EXPECT_LT(mv, SENSOR_FLOATING_LEVEL_MV);

        int32_t result = proc.get_temperature_x10(mv);
        EXPECT_NEAR(result, temp * 10, 20);
    }
}

//=============================================================================
// TCR Mode Tests
//=============================================================================

TEST(TemperatureProcessorTest, TCR_NoCalibration_Default) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);
    // No calibration - uses defaults: R_DEFAULT=4000mΩ at T_REF=25°C

    // At reference temperature (25°C), resistance should be R_DEFAULT
    int32_t result = proc.get_temperature_x10(TemperatureProcessor::TCR_R_DEFAULT);
    EXPECT_NEAR(result, 25 * 10, 10);

    // Test temperature change using copper physics: R(T) = R0 * (1 + α*(T-T0))
    // Copper: α = 0.00393 [1/°C]
    // At T=125°C: R = 4000 * (1 + 0.00393*(125-25)) = 5572 mΩ
    // ΔR = 1572 mΩ
    uint32_t r_125c = TemperatureProcessor::TCR_R_DEFAULT + 1572;
    result = proc.get_temperature_x10(r_125c);
    EXPECT_NEAR(result, 125 * 10, 10);
}

TEST(TemperatureProcessorTest, TCR_OnePointCalibration) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);

    // Calibrate at known point: 3500mΩ at 100°C
    // This sets new base point but keeps default copper TCR
    proc.set_cal_points(100.0f, 3500.0f, 0.0f, 0.0f);

    // Should read 100°C at calibration resistance
    int32_t result = proc.get_temperature_x10(3500);
    EXPECT_NEAR(result, 100 * 10, 5);

    // Test at higher temperature using copper physics
    // R(T) = R0 * (1 + α*(T-T0))
    // At T=150°C: R = 3500 * (1 + 0.00393*(150-100)) = 4188 mΩ
    // ΔR = 688 mΩ
    result = proc.get_temperature_x10(3500 + 688);
    EXPECT_NEAR(result, 150 * 10, 10);
}

TEST(TemperatureProcessorTest, TCR_TwoPointCalibration) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);

    // Real heater has different TCR than default copper (e.g. tungsten-based)
    // Measured: 3000mΩ at 50°C and 4500mΩ at 200°C
    // Actual α = (4500-3000)/(3000*(200-50)) = 0.00333 [1/°C]
    // Copper α = 0.00393 [1/°C] (15% difference!)

    // BEFORE calibration: using default copper α (wrong for tungsten)
    // Copper model predicts at 3000mΩ: T = 25 + (3000/4000 - 1)/0.00393 ≈ -39°C (WRONG!)
    int32_t result_3000_before = proc.get_temperature_x10(3000);
    int32_t result_3750_before = proc.get_temperature_x10(3750);
    int32_t result_4500_before = proc.get_temperature_x10(4500);

    // Defaults are for copper at 4000mΩ/25°C, so these resistances are way off
    EXPECT_LT(result_3000_before, 50 * 10);  // Will read much lower than 50°C
    EXPECT_LT(result_4500_before, 200 * 10); // Will read much lower than 200°C

    // AFTER 2-point calibration with correct tungsten data
    proc.set_cal_points(50.0f, 3000.0f, 200.0f, 4500.0f);

    // Test at calibration points - should be corrected
    int32_t result_3000_after = proc.get_temperature_x10(3000);
    EXPECT_NEAR(result_3000_after, 50 * 10, 5);

    int32_t result_4500_after = proc.get_temperature_x10(4500);
    EXPECT_NEAR(result_4500_after, 200 * 10, 5);

    // Test at intermediate point: 3750mΩ should be 125°C
    // Linear interpolation: (3750-3000)/(4500-3000) = 0.5 => 50 + 0.5*(200-50) = 125°C
    int32_t result_3750_after = proc.get_temperature_x10(3750);
    EXPECT_NEAR(result_3750_after, 125 * 10, 10);

    // Verify calibration improved readings (moved them higher)
    EXPECT_GT(result_3000_after, result_3000_before);
    EXPECT_GT(result_3750_after, result_3750_before);
    EXPECT_GT(result_4500_after, result_4500_before);
}

//=============================================================================
// Division by Zero Protection Tests
//=============================================================================

TEST(TemperatureProcessorTest, DivByZero_RTD_IdenticalADCValues) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);

    uint32_t mv_100c = temp_to_mv(100 * 10);

    // Set two calibration points with identical ADC values (different temps)
    proc.set_cal_points(100.0f, static_cast<float>(mv_100c),
                       200.0f, static_cast<float>(mv_100c));

    // Should not crash and should behave like 1-point calibration
    int32_t result = proc.get_temperature_x10(mv_100c);
    EXPECT_NEAR(result, 100 * 10, 10);
}

TEST(TemperatureProcessorTest, DivByZero_RTD_IdenticalTemperatures) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);

    uint32_t mv_100c = temp_to_mv(100 * 10);
    uint32_t mv_200c = temp_to_mv(200 * 10);

    // Set two calibration points with identical temperatures (different ADC)
    proc.set_cal_points(100.0f, static_cast<float>(mv_100c),
                       100.0f, static_cast<float>(mv_200c));

    // Should not crash and should behave like 1-point calibration
    int32_t result = proc.get_temperature_x10(mv_100c);
    EXPECT_NEAR(result, 100 * 10, 10);
}

TEST(TemperatureProcessorTest, DivByZero_TCR_IdenticalResistances) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);

    // Set two calibration points with identical resistances
    proc.set_cal_points(100.0f, 3500.0f, 200.0f, 3500.0f);

    // Should not crash and should behave like 1-point calibration
    int32_t result = proc.get_temperature_x10(3500);
    EXPECT_NEAR(result, 100 * 10, 10);
}

TEST(TemperatureProcessorTest, DivByZero_TCR_IdenticalTemperatures) {
    TemperatureProcessor proc;
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);

    // Set two calibration points with identical temperatures
    proc.set_cal_points(100.0f, 3000.0f, 100.0f, 4500.0f);

    // Should not crash and should behave like 1-point calibration
    int32_t result = proc.get_temperature_x10(3000);
    EXPECT_NEAR(result, 100 * 10, 10);
}

TEST(TemperatureProcessorTest, DivByZero_CompletelyIdenticalPoints) {
    TemperatureProcessor proc;

    // RTD mode
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);
    uint32_t mv = temp_to_mv(100 * 10);
    proc.set_cal_points(100.0f, static_cast<float>(mv), 100.0f, static_cast<float>(mv));
    int32_t result = proc.get_temperature_x10(mv);
    EXPECT_NEAR(result, 100 * 10, 10);

    // TCR mode
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);
    proc.set_cal_points(150.0f, 4000.0f, 150.0f, 4000.0f);
    result = proc.get_temperature_x10(4000);
    EXPECT_NEAR(result, 150 * 10, 10);
}

//=============================================================================
// Sensor Type Switching Tests
//=============================================================================

TEST(TemperatureProcessorTest, SwitchSensorType_RecalculatesCoefficients) {
    TemperatureProcessor proc;

    // Start with RTD
    proc.set_sensor_type(TemperatureProcessor::SensorType::RTD);
    uint32_t mv = temp_to_mv(100 * 10);
    proc.set_cal_points(100.0f, static_cast<float>(mv), 0.0f, 0.0f);

    int32_t result_rtd = proc.get_temperature_x10(mv);
    EXPECT_NEAR(result_rtd, 100 * 10, 10);

    // Switch to TCR with different calibration
    proc.set_sensor_type(TemperatureProcessor::SensorType::TCR);
    proc.set_cal_points(150.0f, 4000.0f, 0.0f, 0.0f);

    int32_t result_tcr = proc.get_temperature_x10(4000);
    EXPECT_NEAR(result_tcr, 150 * 10, 10);

    // Values should be completely different
    EXPECT_NE(result_rtd, result_tcr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
