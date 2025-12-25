#pragma once

#include <etl/algorithm.h>
#include <etl/limits.h>
#include <etl/vector.h>
#include <pd/pd.h>

#include "lib/isqrt.hpp"

class ProfileSelector {
public:
    static constexpr uint32_t DEFAULT_MV_DESIRED = 5500;
    static constexpr uint32_t DEFAULT_MV_FALLBACK = 5000;

    struct PDO_DESCRIPTOR {
        pd::PDO_VARIANT pdo_variant{pd::PDO_VARIANT::UNKNOWN};

        uint32_t mv_min{0};
        uint32_t mv_max{0};
        uint32_t ma_max{0};

        // For empty record make valid resistance unreachable
        uint32_t mohms_min{etl::numeric_limits<uint32_t>::max()};
    };

    struct POWER_PLAN {
        uint32_t profile_idx{0}; // 0-based
        uint32_t mv{0};
        uint32_t duty_x1000{0};
        uint32_t ctx_target_mw{0};
    };

    struct FEEDBACK_PARAMS {
        // Measurements
        uint32_t peak_mv{0};
        uint32_t peak_ma{0};
        // Measurements context (for advanced correction)
        uint32_t req_mv{0};
        uint32_t req_idx{0};
    };

    enum PowerStrategy {
        ST_UNKNOWN,
        ST_UP,
        ST_HOLD,
        ST_DOWN
    };

    PowerStrategy power_strategy{ST_UNKNOWN};

    etl::vector<PDO_DESCRIPTOR, pd::MaxPdoObjects> descriptors;
    uint32_t current_index{0};

    // Minimal profile (voltage) to use (updated on src caps load).
    // - 5500 mV when APDO available
    // - 5000 mV otherwise (first PDO)
    uint32_t default_position{1};
    uint32_t default_mv{DEFAULT_MV_FALLBACK};

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
                    break;

                default:
                    // Do nothing, will push default empty object to keep indexes
                    break;
            }

            descriptors.push_back(desc);
        }

        default_position = 1;
        default_mv = DEFAULT_MV_FALLBACK;

        // Try to find APDO with DEFAULT_MV_DESIRED support
        if (!descriptors.empty()) {
            for (size_t i = descriptors.size()-1; i > 0; i--) {
                auto& d = descriptors[i];
                if (d.mv_max >= DEFAULT_MV_DESIRED && d.mv_min <= DEFAULT_MV_DESIRED) {
                    default_position = i + 1;
                    default_mv = DEFAULT_MV_DESIRED;
                    break;
                }
            }
        }

        set_pdo_index(0);
    };

    auto set_power_strategy(PowerStrategy strategy) -> ProfileSelector& {
        power_strategy = strategy;
        return *this;
    }

    auto set_pdo_index(int32_t index) -> ProfileSelector& {
        current_index = index;
        return *this;
    }

    auto plan_power(uint32_t target_power_mw, const FEEDBACK_PARAMS& feedback) -> POWER_PLAN {
        uint32_t idx = current_index;
        better_pdo_available(target_power_mw, feedback, idx);
        return get_pdo_usage_params(idx, target_power_mw, feedback);
    }

    uint32_t mw_max(uint32_t idx, uint32_t load_mohms) const {
        if (load_mohms == 0) { return 0; }
        auto& d = descriptors[idx];
        return etl::min(d.mv_max * d.mv_max / load_mohms,
            ((d.ma_max * d.ma_max / 1000) * load_mohms) / 1000);
    }

private:
    static constexpr uint32_t PROFILE_UP_SRC_POWER_TERESHOLD_PCT = 5;
    static constexpr uint32_t PROFILE_UP_DST_POWER_RESERVE_PCT = 10;
    static constexpr uint32_t PROFILE_DOWN_DST_POWER_RESERVE_PCT = 40;
    static constexpr uint32_t PROFILE_IN_CURRENT_MARGIN_PCT = 5;
    static constexpr uint32_t PROFILE_OUT_CURRENT_MARGIN_PCT = 2;

    // Select best PDO for target power.
    // - Emergency: overcurrent or APDO trap → drop to PDO[0]
    // - Upgrade: when target > 95% of current max → scan_up with 10% reserve
    // - Downgrade (FIXED only): when current profile has >40% excess → scan_down
    bool better_pdo_available(uint32_t target_power_mw, const FEEDBACK_PARAMS& feedback, uint32_t& out_index) {
        auto load_mohms = get_load_mohms(feedback);
        out_index = current_index;
        if (descriptors.empty() || load_mohms == 0) { return false; }

        uint32_t idx = current_index;

        // Safety check (this should never happen)
        if (idx >= descriptors.size() ||
            descriptors[idx].pdo_variant == pd::PDO_VARIANT::UNKNOWN)
        {
            idx = 0;
        }

        // Emergency case 1: we are close to overcurrent. Force downgrade to
        // first PDO (PD mandates 5 V FIXED at index 0), then continue with exact match.
        // 5% current margin (R < Rmin·1.05) to avoid tripping.
        if (!has_current_margin(idx, load_mohms, PROFILE_OUT_CURRENT_MARGIN_PCT)) {
            idx = 0;
        }

        // Emergency case 2: APDO minimal voltage too high for target to stay
        // without PWM (and profile s with lower voltage available. Drop to a
        // safe base to avoid PWM lock.
        if (is_apdo_min_voltage_trap(idx, load_mohms, target_power_mw)) {
            idx = 0;
        }

        // If desired power is close to PDO limit - try to upgrade
        // Upgrade when target exceeds 95% of current Pmax (5% headroom).
        if (target_power_mw > mw_max_with_reserve(idx, load_mohms, PROFILE_UP_SRC_POWER_TERESHOLD_PCT)) {
            out_index = scan_up(idx, load_mohms, target_power_mw);
            return out_index != current_index;
        }

        // Downgrade: only for FIXED profiles, if lower profile can handle target
        // with 40% reserve. Such reserve is set to prevent oscillation near
        // profile boundaries.
        if (descriptors[idx].pdo_variant == pd::PDO_VARIANT::FIXED) {
            auto downgrade_candidate = scan_down(idx, load_mohms, target_power_mw);
            if (downgrade_candidate != idx) {
                out_index = downgrade_candidate;
                return out_index != current_index;
            }
        }

        // We can come here in 2 cases:
        // - no change needed
        // - emergencies updated new_index (e.g. to 0 → 5 V FIXED per PD),
        //   but no better profile available. Apply this properly.
        out_index = idx;
        return out_index != current_index;
    }

    auto get_pdo_usage_params(uint32_t idx, uint32_t target_power_mw, const FEEDBACK_PARAMS& feedback) -> POWER_PLAN {
        POWER_PLAN params{};
        params.profile_idx = idx;
        params.ctx_target_mw = target_power_mw;

        auto load_mohms = get_load_mohms(feedback);
        if (idx >= descriptors.size() || load_mohms == 0) {
            return params;
        }

        auto& d = descriptors[idx];

        // Calculate voltage to get target power or max available
        if (d.pdo_variant == pd::PDO_VARIANT::FIXED) {
            auto mv = d.mv_min;
            auto max_mw = (mv * mv) / load_mohms;

            if (target_power_mw > max_mw) {
                params.duty_x1000 = 1000;
            } else {
                if (max_mw == 0) {
                    // This is dead branch, formal check for division by zero
                    params.duty_x1000 = 0;
                } else {
                    params.duty_x1000 = target_power_mw * 1000 / max_mw;
                }
            }

            params.mv = mv;
            return params;
        }

        if (is_apdo(d.pdo_variant)) {
            // For APDO, calculate voltage to get target power: V = sqrt(P * R)
            auto possible_mw = etl::min(target_power_mw, mw_max(idx, load_mohms));
            // 32 bits should not overflow but use 64 bits for sure.
            // Sqrt speed will be ~ the same
            uint32_t ideal_mv = isqrt64(static_cast<uint64_t>(possible_mw) * load_mohms);

            params.mv = etl::clamp<uint32_t>(ideal_mv, d.mv_min, d.mv_max);
            if (params.mv < default_mv) { params.mv = default_mv; }

            auto max_duty_mw = (params.mv * params.mv / load_mohms);

            if (max_duty_mw == 0) {
                // This is dead branch, formal check for division by zero
                params.duty_x1000 = 0;
            } else {
                params.duty_x1000 = possible_mw * 1000 / max_duty_mw;
            }

            // Add this to cut down on rounding errors
            if (params.duty_x1000 > 1000) { params.duty_x1000 = 1000; }

            return params;
        }

        // Unknown PDO type, return zeroes
        return params;
    }

    // Compute load_mohms from feedback
    uint32_t get_load_mohms(const FEEDBACK_PARAMS& feedback) const {
        if (feedback.peak_ma == 0) { return 0; }
        return feedback.peak_mv * 1000 / feedback.peak_ma;
    }

    static bool is_apdo(pd::PDO_VARIANT v) {
        return v == pd::PDO_VARIANT::APDO_PPS ||
               v == pd::PDO_VARIANT::APDO_SPR_AVS ||
               v == pd::PDO_VARIANT::APDO_EPR_AVS;
    }

    // Check if APDO would require PWM at its min voltage (trap condition).
    // If minV is above default_mv and P(minV) > target·1.03, we risk PWM lock.
    bool is_apdo_min_voltage_trap(uint32_t idx, uint32_t load_mohms, uint32_t target_mw) const {
        auto& d = descriptors[idx];
        if (!is_apdo(d.pdo_variant)) { return false; }
        if (d.mv_min <= default_mv) { return false; }
        auto min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms;
        return min_continuous_mw > (target_mw * 103 / 100);
    }

    // Check if profile has enough current margin.
    bool has_current_margin(uint32_t idx, uint32_t load_mohms, uint32_t margin_pct) const {
        auto required_mohms = static_cast<uint64_t>(descriptors[idx].mohms_min)
            * (100 + margin_pct) / 100;
        return load_mohms >= required_mohms;
    }

    // Returns max power with specified reserve percentage.
    // reserve_pct=10 means profile must provide target + 10% headroom.
    uint32_t mw_max_with_reserve(uint32_t idx, uint32_t load_mohms, uint32_t reserve_pct) const {
        return mw_max(idx, load_mohms) * (100 - reserve_pct) / 100;
    }

    // Find profile with more power. Returns index or current if none found.
    // Prefers APDO over Fixed. Requires 10% power headroom.
    uint32_t scan_up(uint32_t from_idx, uint32_t load_mohms, uint32_t target_mw) const {
        // First pass: find suitable APDO (preferred to avoid PWM)
        for (int i = descriptors.size() - 1; i >= 0; i--) {
            auto& d = descriptors[i];
            if (!is_apdo(d.pdo_variant)) { continue; }
            if (!has_current_margin(i, load_mohms, PROFILE_IN_CURRENT_MARGIN_PCT)) { continue; }
            if (is_apdo_min_voltage_trap(i, load_mohms, target_mw)) { continue; }
            if (target_mw <= mw_max_with_reserve(i, load_mohms, PROFILE_UP_DST_POWER_RESERVE_PCT)) {
                return i;
            }
        }

        // Second pass: find any PDO with enough power
        uint32_t best_idx = from_idx;
        uint32_t best_mw = mw_max_with_reserve(from_idx, load_mohms, PROFILE_UP_DST_POWER_RESERVE_PCT);

        for (uint32_t i = 0; i < descriptors.size(); i++) {
            auto& d = descriptors[i];
            if (d.pdo_variant == pd::PDO_VARIANT::UNKNOWN) { continue; }
            if (!has_current_margin(i, load_mohms, PROFILE_IN_CURRENT_MARGIN_PCT)) { continue; }
            if (is_apdo_min_voltage_trap(i, load_mohms, target_mw)) { continue; }

            auto mw_tmp = mw_max_with_reserve(i, load_mohms, PROFILE_UP_DST_POWER_RESERVE_PCT);
            if (mw_tmp > best_mw) {
                best_idx = i;
                best_mw = mw_tmp;
            }
            if (mw_tmp > target_mw) {
                return i;
            }
        }

        return best_idx;
    }

    // Find lower-power profile with sufficient reserve.
    // Prefers APDO over Fixed. Requires 40% power headroom to prevent oscillation.
    uint32_t scan_down(uint32_t from_idx, uint32_t load_mohms, uint32_t target_mw) const {
        // First pass: find suitable APDO (preferred to avoid PWM)
        for (uint32_t i = 0; i < descriptors.size(); i++) {
            auto& d = descriptors[i];
            if (!is_apdo(d.pdo_variant)) { continue; }
            if (!has_current_margin(i, load_mohms, PROFILE_IN_CURRENT_MARGIN_PCT)) { continue; }
            if (is_apdo_min_voltage_trap(i, load_mohms, target_mw)) { continue; }
            if (target_mw <= mw_max_with_reserve(i, load_mohms, PROFILE_DOWN_DST_POWER_RESERVE_PCT)) {
                return i;
            }
        }

        // Second pass: find lower-voltage Fixed
        auto current_mv = descriptors[from_idx].mv_max;
        for (uint32_t i = 0; i < descriptors.size(); i++) {
            auto& d = descriptors[i];
            if (d.pdo_variant != pd::PDO_VARIANT::FIXED) { continue; }
            if (!has_current_margin(i, load_mohms, PROFILE_IN_CURRENT_MARGIN_PCT)) { continue; }
            if (d.mv_max >= current_mv) { continue; }
            if (target_mw <= mw_max_with_reserve(i, load_mohms, PROFILE_DOWN_DST_POWER_RESERVE_PCT)) {
                return i;
            }
        }

        return from_idx;
    }
};
