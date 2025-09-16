#pragma once

#include <etl/algorithm.h>
#include <etl/limits.h>
#include <etl/vector.h>
#include <pd/pd.h>

class ProfileSelector {
public:
    struct PDO_DESCRIPTOR {
        pd::PDO_VARIANT pdo_variant{pd::PDO_VARIANT::UNKNOWN};

        uint32_t mv_min{0};
        uint32_t mv_max{0};
        uint32_t ma_max{0};

        // For empty record make valid resistance unreachable
        uint32_t mohms_min{etl::numeric_limits<uint32_t>::max()};
        uint32_t mohms_min_105_percent{etl::numeric_limits<uint32_t>::max()};
        uint32_t mohms_min_110_percent{etl::numeric_limits<uint32_t>::max()};
    };

    etl::vector<PDO_DESCRIPTOR, pd::MaxPdoObjects> descriptors;
    int32_t current_index{0};
    int32_t better_index{0};
    uint32_t load_mohms{0};
    uint32_t target_power_mw{0};

    void load_pdos(pd::PDO_LIST const& pdos) {
        using namespace pd::dobj_utils;
        descriptors.clear();
        for (auto pdo : pdos) {
            PDO_DESCRIPTOR desc{};
            auto pdo_variant = get_src_pdo_variant(pdo);
            auto limits = get_src_pdo_limits(pdo);

            switch (pdo_variant) {
                case pd::PDO_VARIANT::APDO_PPS: ETL_FALLTHROUGH;
                case pd::PDO_VARIANT::APDO_SPR_AVS: ETL_FALLTHROUGH;
                case pd::PDO_VARIANT::APDO_EPR_AVS: ETL_FALLTHROUGH;
                case pd::PDO_VARIANT::FIXED:
                    desc.pdo_variant = pdo_variant;
                    // Clamp to ≥5 V: PD 3.2 bans <5 V, and our HW requires ≥5 V.
                    desc.mv_min = limits.mv_min < 5000 ? 5000 : limits.mv_min;
                    desc.mv_max = limits.mv_max;
                    desc.ma_max = limits.ma > 0
                        ? limits.ma
                        : (limits.pdp * 1000 * 1000 / limits.mv_max);
                    desc.mohms_min = (desc.mv_min * 1000) / desc.ma_max;
                    desc.mohms_min_105_percent = desc.mohms_min * 105 / 100;
                    desc.mohms_min_110_percent = desc.mohms_min * 110 / 100;
                    break;

                default:
                    // Do nothing, will push default empty object to keep indexes
                    break;
            }

            descriptors.push_back(desc);
        }
    };

    auto set_load_mohms(uint32_t R_mohms) -> ProfileSelector& {
        load_mohms = R_mohms;
        return *this;
    }

    auto set_target_power_mw(uint32_t power_mw) -> ProfileSelector& {
        target_power_mw = power_mw;
        return *this;
    }

    auto set_pdo_index(int32_t index) -> ProfileSelector& {
        current_index = index;
        better_index = index;
        return *this;
    }

    uint32_t mw_max(uint32_t idx) const {
        auto& d = descriptors[idx];
        return etl::min(d.mv_max * d.mv_max / load_mohms,
            ((d.ma_max * d.ma_max / 1000) * load_mohms) / 1000);
    }

    uint32_t mw_max_95_percent(uint32_t idx) const {
        return mw_max(idx) * 95 / 100;
    }

    uint32_t mw_max_90_percent(uint32_t idx) const {
        return mw_max(idx) * 90 / 100;
    }

    
    // Hysteresis and guards, to avoid oscillation and PWM noise:
    // - Upgrade when target > 95% of current Pmax (5% headroom left).
    // - Candidates must provide ≥10% headroom (power and current checks).
    // - Emergency downgrade when R < Rmin·1.05 (5% current margin).
    // - APDO guard: AVS often min at ~9 V. We clamp minV to 5 V overall;
    //   if APDO's minV is above 5 V and P(minV) > target·1.03, we'd need PWM
    //   and have no overcurrent trigger to step down — avoid/leave such APDO.
    // - Prefer APDO when possible; for FIXED try lower voltage if it still meets target.
    bool better_pdo_available() {
        if (descriptors.empty() || load_mohms == 0) { return false; }

        uint32_t new_index = current_index;

        // Emergency case 1: we are close to overcurrent. Force downgrade to
        // first PDO (PD mandates 5 V FIXED at index 0), then continue with exact match.
        // 5% current margin (R < Rmin·1.05) to avoid tripping.
        if (load_mohms < descriptors[new_index].mohms_min_105_percent) {
            new_index = 0;
        }

        // Emergency case 2: APDO/AVS minimal voltage too high for target
        // AVS often min at ~9 V. Since we clamp to ≥5 V, if APDO's minV > 5 V
        // and P(minV) > target·1.03, we'd sit on PWM with no path to step down.
        // Drop to a safe base to avoid PWM lock.
        {
            auto& d = descriptors[new_index];
            auto min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms;
            if ((d.pdo_variant == pd::PDO_VARIANT::APDO_SPR_AVS ||
                d.pdo_variant == pd::PDO_VARIANT::APDO_EPR_AVS) &&
                d.mv_min > 5000 &&
                min_continuous_mw > (target_power_mw * 103 / 100))
            {
                new_index = 0;
            }
        }


        // If desired power is close to PDO limit - try to upgrade
        // Upgrade when target exceeds 95% of current Pmax (5% headroom).
        if (target_power_mw > mw_max_95_percent(new_index)) {

            // First, try to find APDO. Satisfying 2 conditions:
            // - It can get desired power
            // - it not fall to PWM if min voltage > 5.0v
            for (int i = descriptors.size() - 1; i >= 0; i--) {
                auto& d = descriptors[i];
                if (d.pdo_variant != pd::PDO_VARIANT::APDO_PPS &&
                    d.pdo_variant != pd::PDO_VARIANT::APDO_SPR_AVS &&
                    d.pdo_variant != pd::PDO_VARIANT::APDO_EPR_AVS)
                {
                    continue;
                }
                if (load_mohms < d.mohms_min_110_percent) { continue; } // require 10% current margin

                // Avoid APDO that would require PWM at minV (>5 V): AVS often ~9 V.
                // If P(minV) > target·1.03 we risk PWM lock with no down-step trigger.
                auto min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms;
                if (d.mv_min > 5000 && min_continuous_mw > (target_power_mw * 103 / 100)) {
                    continue;
                }

                if (target_power_mw <= mw_max_90_percent(i)) { // candidate has ≥10% headroom
                    better_index = i;
                    return better_index != current_index;
                }
            }

            // If no APDO matched, try to find any PDO with higher power
            // above target
            auto current_max_mw = mw_max_90_percent(new_index); // compare with 10% hysteresis

            for (uint8_t i = 0; i < descriptors.size(); i++) {
                auto& d = descriptors[i];
                if (d.pdo_variant == pd::PDO_VARIANT::UNKNOWN) { continue; }
                if (load_mohms < d.mohms_min_110_percent) { continue; } // require 10% current margin

                // Decline more weak profiles
                auto mw_tmp = mw_max_90_percent(i); // consider 10% headroom
                if (mw_tmp > current_max_mw) { new_index = i; }

                // Avoid APDO needing PWM at minV (>5 V): if P(minV) > target·1.03
                // we might get “stuck” in APDO without an overcurrent downgrade.
                if ((d.pdo_variant == pd::PDO_VARIANT::APDO_PPS ||
                     d.pdo_variant == pd::PDO_VARIANT::APDO_SPR_AVS ||
                     d.pdo_variant == pd::PDO_VARIANT::APDO_EPR_AVS))
                {
                    auto min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms;
                    if (d.mv_min > 5000 && min_continuous_mw > (target_power_mw * 103 / 100)) {
                        continue;
                    }
                }
                if (mw_tmp > target_power_mw) { // candidate exceeds target with 10% headroom
                    better_index = i;
                    return better_index != current_index;
                }
            }

            // If target power not satisfied, use the best we have
            better_index = new_index;
            return better_index != current_index;
        }

        // For fixed profiles - try to downgrade to lower voltage if possible.
        if (descriptors[new_index].pdo_variant == pd::PDO_VARIANT::FIXED) {
            // First, try to find suitable APDO to avoid PWM.
            for (uint8_t i = 0; i < descriptors.size(); i++) {
                auto& d = descriptors[i];
                if (d.pdo_variant != pd::PDO_VARIANT::APDO_PPS &&
                    d.pdo_variant != pd::PDO_VARIANT::APDO_SPR_AVS &&
                    d.pdo_variant != pd::PDO_VARIANT::APDO_EPR_AVS)
                {
                    continue;
                }
                if (load_mohms < d.mohms_min_110_percent) { continue; } // require 10% current margin

                auto mw_tmp = mw_max_90_percent(i); // needs 10% power headroom
                if (mw_tmp < target_power_mw) { continue; }

                // Avoid APDO needing PWM at minV (>5 V): 3% margin
                auto min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms;
                if (d.mv_min > 5000 && min_continuous_mw > (target_power_mw * 103 / 100)) {
                    continue;
                }

                better_index = i;
                return better_index != current_index;
            }

            // No suitable APDO, try to find lower fixed PDO
            for (uint8_t i = 0; i < descriptors.size(); i++) {
                auto& d = descriptors[i];
                if (d.pdo_variant != pd::PDO_VARIANT::FIXED) { continue; }
                if (load_mohms < d.mohms_min_110_percent) { continue; } // require 10% current margin
                if (d.mv_max >= descriptors[new_index].mv_max) { continue; } // only lower-voltage FIXED
                if (target_power_mw <= mw_max_90_percent(i)) { // supports target with 10% headroom
                    better_index = i;
                    return better_index != current_index;
                }
            }
        }

        // We can come here in 2 cases:
        // - no change needed
        // - emergencies updated new_index (e.g. to 0 → 5 V FIXED per PD),
        //   but no better profile available. Apply this properly.
        better_index = new_index;
        return better_index != current_index;
    }
};
