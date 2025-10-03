#include <gtest/gtest.h>
#include "lib/pt100.hpp"

TEST(PT100Test, ExactTableValues) {
    // Test exact table values from PDF
    EXPECT_EQ(PT100::r2t_x10(100000), 0 * 10);      // 0°C = 100.000Ω
    EXPECT_EQ(PT100::r2t_x10(119397), 50 * 10);     // 50°C = 119.397Ω
    EXPECT_EQ(PT100::r2t_x10(138505), 100 * 10);    // 100°C = 138.505Ω
    EXPECT_EQ(PT100::r2t_x10(212052), 300 * 10);    // 300°C = 212.052Ω
    EXPECT_EQ(PT100::r2t_x10(88222), -30 * 10);     // -30°C = 88.222Ω
}

TEST(PT100Test, InterpolatedValues) {
    // Test intermediate values that require interpolation

    // Between 0°C and 10°C: test 5°C
    EXPECT_NEAR(PT100::r2t_x10(101951), 5 * 10, 1);     // 5°C ≈ 101.951Ω

    // Between 20°C and 30°C: test 25°C
    EXPECT_NEAR(PT100::r2t_x10(109734), 25 * 10, 1);    // 25°C ≈ 109.734Ω

    // Between 80°C and 90°C: test 85°C
    EXPECT_NEAR(PT100::r2t_x10(132802), 85 * 10, 1);    // 85°C ≈ 132.802Ω

    // Between 150°C and 160°C: test 155°C
    EXPECT_NEAR(PT100::r2t_x10(159190), 155 * 10, 1);   // 155°C ≈ 159.190Ω

    // Between 290°C and 300°C: test 295°C
    EXPECT_NEAR(PT100::r2t_x10(210268), 295 * 10, 1);   // 295°C ≈ 210.268Ω
}

TEST(PT100Test, NegativeInterpolation) {
    // Test negative temperature interpolation

    // Between -50°C and -25°C
    EXPECT_NEAR(PT100::r2t_x10(88222), -30 * 10, 1);    // -30°C = 88.222Ω

    // Between -100°C and -50°C
    EXPECT_NEAR(PT100::r2t_x10(72335), -70 * 10, 1);   // -70°C = 72.335Ω
}

TEST(PT100Test, EdgeCases) {
    // Out of range - should return boundary values
    EXPECT_EQ(PT100::r2t_x10(18520), -200 * 10);   // Exact lower bound
    EXPECT_EQ(PT100::r2t_x10(390481), 850 * 10);   // Exact upper bound

    // Very out of range
    EXPECT_EQ(PT100::r2t_x10(10000), -200 * 10);   // Below table range
    EXPECT_EQ(PT100::r2t_x10(500000), 850 * 10);   // Above table range
}

// Inverse function tests: temperature to resistance
TEST(PT100Test, InverseExactTableValues) {
    // Test exact table values - inverse of r2t_x10
    EXPECT_EQ(PT100::x10_t2r(0 * 10), 100000u);      // 0°C = 100.000Ω
    EXPECT_EQ(PT100::x10_t2r(50 * 10), 119397u);     // 50°C = 119.397Ω
    EXPECT_EQ(PT100::x10_t2r(100 * 10), 138505u);    // 100°C = 138.505Ω
    EXPECT_EQ(PT100::x10_t2r(300 * 10), 212052u);    // 300°C = 212.052Ω
    EXPECT_EQ(PT100::x10_t2r(-30 * 10), 88222u);     // -30°C = 88.222Ω
}

TEST(PT100Test, InverseInterpolatedValues) {
    // Test intermediate values that require interpolation

    // Between 0°C and 10°C: test 5°C
    EXPECT_NEAR(PT100::x10_t2r(5 * 10), 101951u, 1);     // 5°C ≈ 101.951Ω

    // Between 20°C and 30°C: test 25°C
    EXPECT_NEAR(PT100::x10_t2r(25 * 10), 109734u, 1);    // 25°C ≈ 109.734Ω

    // Between 80°C and 90°C: test 85°C
    EXPECT_NEAR(PT100::x10_t2r(85 * 10), 132802u, 1);    // 85°C ≈ 132.802Ω

    // Between 150°C and 160°C: test 155°C
    EXPECT_NEAR(PT100::x10_t2r(155 * 10), 159190u, 1);   // 155°C ≈ 159.190Ω

    // Between 290°C and 300°C: test 295°C
    EXPECT_NEAR(PT100::x10_t2r(295 * 10), 210268u, 1);   // 295°C ≈ 210.268Ω
}

TEST(PT100Test, InverseNegativeInterpolation) {
    // Test negative temperature interpolation

    // Between -50°C and -25°C
    EXPECT_NEAR(PT100::x10_t2r(-30 * 10), 88222u, 1);    // -30°C = 88.222Ω

    // Between -100°C and -50°C
    EXPECT_NEAR(PT100::x10_t2r(-70 * 10), 72335u, 1);    // -70°C = 72.335Ω
}

TEST(PT100Test, InverseEdgeCases) {
    // Out of range - should return boundary values
    EXPECT_EQ(PT100::x10_t2r(-200 * 10), 18520u);    // Exact lower bound
    EXPECT_EQ(PT100::x10_t2r(850 * 10), 390481u);    // Exact upper bound

    // Very out of range
    EXPECT_EQ(PT100::x10_t2r(-300 * 10), 18520u);    // Below table range
    EXPECT_EQ(PT100::x10_t2r(1000 * 10), 390481u);   // Above table range
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
