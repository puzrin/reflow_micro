#include <gtest/gtest.h>
#include "lib/adc_interpolator.hpp"

// ESP32 12-bit ADC (raw: 0-4095) with 0dB attenuation (ADC_ATTEN_DB_0)
// Full-scale voltage: 1100mV (recommended accurate range: 100-950mV)

TEST(AdcInterpolatorTest, EdgeCases) {
    AdcInterpolator<10> interp;

    // Sample calibration table
    interp.points.push_back({1000, 231});  // 231mV at raw=1000
    interp.points.push_back({2000, 463});  // 463mV at raw=2000
    interp.points.push_back({3000, 695});  // 695mV at raw=3000

    // Below first point: should return first value
    EXPECT_EQ(interp.to_uv(500, 1), 231000u);  // 231mV

    // Above last point: should return last value
    EXPECT_EQ(interp.to_uv(3500, 1), 695000u);  // 695mV
}

TEST(AdcInterpolatorTest, ExactPoints) {
    AdcInterpolator<10> interp;

    interp.points.push_back({1000, 231});
    interp.points.push_back({2000, 463});
    interp.points.push_back({3000, 695});

    // Exact match on points
    EXPECT_EQ(interp.to_uv(1000, 1), 231000u);
    EXPECT_EQ(interp.to_uv(2000, 1), 463000u);
    EXPECT_EQ(interp.to_uv(3000, 1), 695000u);
}

TEST(AdcInterpolatorTest, Interpolation) {
    AdcInterpolator<10> interp;

    // Three points to test search across two intervals
    interp.points.push_back({1000, 231});  // 231mV at raw=1000
    interp.points.push_back({2000, 463});  // 463mV at raw=2000
    interp.points.push_back({3000, 695});  // 695mV at raw=3000

    // First interval [1000, 2000]: midpoint at raw=1500
    // Expected: 231 + (463-231)/2 = 347mV
    EXPECT_EQ(interp.to_uv(1500, 1), 347000u);

    // Second interval [2000, 3000]: midpoint at raw=2500
    // Expected: 463 + (695-463)/2 = 579mV
    EXPECT_EQ(interp.to_uv(2500, 1), 579000u);
}

TEST(AdcInterpolatorTest, ScaledInterpolation) {
    AdcInterpolator<10> interp;

    interp.points.push_back({2000, 463});
    interp.points.push_back({2200, 520});
    interp.points.push_back({2400, 558});

    // First interval [2000, 2200]: sum=21050, count=10 → avg=2105
    // numerator = 21050 - 20000 = 1050
    // denominator = 22000 - 20000 = 2000
    // result = 463 + 57 * 1050/2000 = 463 + 29.925 = 492.925mV = 492925uV
    EXPECT_EQ(interp.to_uv(21050, 10), 492925u);

    // Second interval [2200, 2400]: sum=23100, count=10 → avg=2310
    // numerator = 23100 - 22000 = 1100
    // denominator = 24000 - 22000 = 2000
    // result = 520 + 38 * 1100/2000 = 520 + 20.9 = 540.9mV = 540900uV
    EXPECT_EQ(interp.to_uv(23100, 10), 540900u);

    // Close to boundary: sum=21997, count=10 → avg=2199.7
    // First interval, near right edge
    // numerator = 21997 - 20000 = 1997
    // denominator = 2000
    // result = 463 + 57 * 1997/2000 = 463 + 56.9145 = 519.9145mV = 519914uV (rounded down)
    EXPECT_EQ(interp.to_uv(21997, 10), 519914u);

    // Fractional sum: sum=20133, count=10 → avg=2013.3
    // numerator = 20133 - 20000 = 133
    // denominator = 2000
    // result = 463 + 57 * 133/2000 = 463 + 3.7905 = 466.7905mV = 466790uV (rounded down)
    EXPECT_EQ(interp.to_uv(20133, 10), 466790u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
