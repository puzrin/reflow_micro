// ProfileSelector tests focus on switching logic. We bypass load_pdos()
// and fill descriptors directly with synthetic but spec-like profiles.

#include <gtest/gtest.h>
#include "heater/profile_selector.hpp"

using pd::PDO_VARIANT;

// Helper: construct descriptor with derived mohms thresholds
static ProfileSelector::PDO_DESCRIPTOR make_desc(
    PDO_VARIANT v, uint32_t mv_min, uint32_t mv_max, uint32_t ma_max)
{
    ProfileSelector::PDO_DESCRIPTOR d{};
    d.pdo_variant = v;
    d.mv_min = mv_min < 5000 ? 5000 : mv_min; // clamp to ≥5 V
    d.mv_max = mv_max;
    d.ma_max = ma_max;
    // Derived thresholds
    if (d.ma_max == 0u) {
        // Treat as unusable/UNKNOWN-like: make thresholds unreachable
        d.mohms_min = etl::numeric_limits<uint32_t>::max();
        d.mohms_min_105_percent = etl::numeric_limits<uint32_t>::max();
        d.mohms_min_110_percent = etl::numeric_limits<uint32_t>::max();
    } else {
        d.mohms_min = (d.mv_min * 1000u) / d.ma_max; // Rmin = Vmin / Imax
        d.mohms_min_105_percent = d.mohms_min * 105u / 100u;
        d.mohms_min_110_percent = d.mohms_min * 110u / 100u;
    }
    return d;
}

TEST(ProfileSelectorTest, UpgradeToAPDOWithHeadroom) {
    ProfileSelector ps{};
    // Typical charger: 5V/3A FIXED, 9V/3A FIXED, PPS 5-11V/5A
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,       5000,  5000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,       9000,  9000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_PPS,    5000, 11000, 5000));

    // Start at 5V FIXED, R=3Ω, target a bit above 95% of 5V Pmax to trigger upgrade
    // Pmax(5V, 3Ω) = 25/3 ≈ 8.333W → 95% ≈ 7.916W. Take 8.0W
    ps.set_pdo_index(0).set_load_mohms(3000).set_target_power_mw(8000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 2); // prefer APDO
}

TEST(ProfileSelectorTest, ApdoMinVGuardForcesSafeFallback) {
    ProfileSelector ps{};
    // AVS APDO with minV=9V and strong current, plus a safe 5V FIXED base
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,        5000,  5000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_SPR_AVS, 9000, 21000, 5000));

    // Current at AVS, low target: at 9V and R=3Ω → P(minV)=27W > 1.03*target(5W)
    ps.set_pdo_index(1).set_load_mohms(3000).set_target_power_mw(5000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 0); // drop to safe base
}

TEST(ProfileSelectorTest, UpgradeToAnyHigherWhenNoApdoMatches) {
    ProfileSelector ps{};
    // 5V/3A, 9V/2A, no APDO
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 5000, 5000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 9000, 9000, 2000));

    // R=6Ω → Pmax(5V)=25/6≈4.166W; 95%≈3.958W → target=4.0W triggers upgrade
    ps.set_pdo_index(0).set_load_mohms(6000).set_target_power_mw(4000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 1); // pick 9V FIXED
}

TEST(ProfileSelectorTest, DowngradeFixedToApdoToAvoidPwm) {
    ProfileSelector ps{};
    // 9V/3A FIXED, plus safe PPS 5-11V/3A
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,    9000,  9000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_PPS, 5000, 11000, 3000));

    // R=20Ω, target=3W (< Pmax(9V)=4.05W). Prefer APDO to avoid PWM on FIXED
    ps.set_pdo_index(0).set_load_mohms(20000).set_target_power_mw(3000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 1);
}

TEST(ProfileSelectorTest, DowngradeFixedToLowerFixedWhenApdoNotSuitable) {
    ProfileSelector ps{};
    // 12V/3A FIXED current, lower 9V/3A FIXED available; APDO present but minV too high (guard)
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,        12000, 12000, 3000)); // current
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,         9000,  9000, 3000)); // desired lower
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_SPR_AVS,  9000, 21000, 5000)); // will be guarded out

    // R=27Ω: Pmax(9V)=81/27=3W; require 10% headroom, so target ≤ 2.7W
    ps.set_pdo_index(0).set_load_mohms(27000).set_target_power_mw(2600);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 1); // lower fixed selected
}

TEST(ProfileSelectorTest, BestEffortWhenTargetTooHigh) {
    ProfileSelector ps{};
    // 5V/3A, 9V/3A, 12V/3A — no APDO
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  5000,  5000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  9000,  9000, 3000));
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 12000, 12000, 3000));

    // Choose a safe load (R high enough to satisfy 110% current margin)
    // but set a target above all profiles' Pmax so selector picks the strongest.
    // For R=30Ω: Pmax(12V)=144/30=4.8W, 9V=2.7W, 5V≈0.83W. Target 10W > all.
    ps.set_pdo_index(0).set_load_mohms(30000).set_target_power_mw(10000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 2); // strongest safe FIXED (12V)
}

TEST(ProfileSelectorTest, BestEffortPrefersPpsOverFixedWhenAllInsufficient) {
    ProfileSelector ps{};
    // SPR-like block: FIXED first, then PPS (per spec ordering in SPR block)
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,    5000,  5000, 3000)); // 5V/3A
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,    9000,  9000, 3000)); // 9V/3A
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,   20000, 20000, 5000)); // 20V/5A
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_PPS, 5000, 21000, 5000)); // PPS 5–21V/5A

    // Choose R so both 20V FIXED and PPS are voltage-limited:
    // R=10Ω → Pmax(20V)=400/10=40W, Pmax(PPS@21V)=441/10≈44.1W.
    // Target set above all to trigger best-effort. PPS should win as strongest
    // and avoids PWM compared to staying on a high FIXED.
    ps.set_pdo_index(0).set_load_mohms(10000).set_target_power_mw(100000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 3); // prefer PPS over 20V FIXED in best-effort
}

TEST(ProfileSelectorTest, CandidateFilteredBy110PercentCurrentMargin) {
    ProfileSelector ps{};
    // SPR FIXED set: 5V/3A, 9V/3A, 12V/3A, 15V/3A
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  5000,  5000, 3000)); // 0
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  9000,  9000, 3000)); // 1 (current)
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 12000, 12000, 3000)); // 2
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 15000, 15000, 3000)); // 3

    // R=5.2Ω: 15V requires R >= 5.5Ω (fails 110%), 12V requires R >= 4.4Ω (passes).
    // target > 95% of 9V@5.2Ω (≈15.58W) → choose 12V, not 15V.
    ps.set_pdo_index(1).set_load_mohms(5200).set_target_power_mw(15000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 2);
}

TEST(ProfileSelectorTest, VoltageVsCurrentLimitSelection) {
    ProfileSelector ps{};
    // 5V/3A base, 9V/2A (current-limited at R<4.5Ω), 12V/3A candidate
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  5000,  5000, 3000)); // 0
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  9000,  9000, 2000)); // 1 (current)
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 12000, 12000, 3000)); // 2 (candidate)

    // R=4.4Ω: 9V Pmax = min(81/4.4≈18.4, 4*4.4=17.6) → 17.6W (I-limit)
    //          12V Pmax = min(144/4.4≈32.7, 9*4.4=39.6) → 32.7W (V-limit)
    // target just above 95% of 17.6W → triggers upgrade to 12V.
    ps.set_pdo_index(1).set_load_mohms(4400).set_target_power_mw(17200);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 2);
}

TEST(ProfileSelectorTest, ApdoPriorityLastWins) {
    ProfileSelector ps{};
    // SPR: FIXED then two PPS (later PPS is stronger by Vmax)
    ps.descriptors.clear();
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,    5000,  5000, 3000)); // 0
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,    9000,  9000, 3000)); // 1 (current)
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_PPS, 5000, 11000, 3000)); // 2
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_PPS, 5000, 21000, 3000)); // 3

    // R=10Ω: Pmax(9V)=8.1W, set target 8.0W to trigger upgrade.
    // Both PPS have ≥10% headroom; scan from end should pick index 3.
    ps.set_pdo_index(1).set_load_mohms(10000).set_target_power_mw(8000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 3);
}

TEST(ProfileSelectorTest, EprHolesAndAvsGuardFallsBackToSafeNonPwm) {
    ProfileSelector ps{};
    ps.descriptors.clear();
    // SPR slots 1..7 (0-based indices 0..6): a few FIXED
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  5000,  5000, 3000)); // 0
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED,  9000,  9000, 3000)); // 1
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 15000, 15000, 3000)); // 2 (candidate)
    ps.descriptors.push_back(make_desc(PDO_VARIANT::FIXED, 20000, 20000, 3000)); // 3
    // Fill to emulate remaining SPR positions (indices 4..6) with UNKNOWN holes
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 4 UNKNOWN
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 5 UNKNOWN
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 6 UNKNOWN

    // EPR slots 8..10 (indices 7..9): UNKNOWN holes; slot 11 (index 10): single EPR AVS
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 7 UNKNOWN
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 8 UNKNOWN
    ps.descriptors.push_back(ProfileSelector::PDO_DESCRIPTOR{}); // 9 UNKNOWN
    ps.descriptors.push_back(make_desc(PDO_VARIANT::APDO_EPR_AVS, 15000, 28000, 5000)); // 10 AVS

    // Start on AVS (index 10). R=30Ω → P(minV_AVS)=225/30=7.5W; target=6W → guard triggers.
    // Among FIXED, 15V gives 7.5W (> target, non-PWM). Expect switch to 15V (index 2).
    ps.set_pdo_index(10).set_load_mohms(30000).set_target_power_mw(6000);

    ASSERT_TRUE(ps.better_pdo_available());
    EXPECT_EQ(ps.better_index, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
