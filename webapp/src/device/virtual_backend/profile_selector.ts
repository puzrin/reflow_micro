// Ported from firmware/src/heater/profile_selector.hpp

import type { POWER_DATA } from './power_data'

export interface PDO_DESCRIPTOR {
  mv_min: number
  mv_max: number
  ma_max: number

  // Computed resistance limits
  mohms_min: number
  mohms_min_105_percent: number
  mohms_min_110_percent: number
}

export interface PDO_USAGE_PARAMS {
  mv: number
  duty_x1000: number
}

export class ProfileSelector {
  static readonly DEFAULT_MV_DESIRED = 5500
  static readonly DEFAULT_MV_FALLBACK = 5000

  descriptors: PDO_DESCRIPTOR[] = []
  current_index: number = 0
  better_index: number = 0
  load_mohms: number = 0
  target_power_mw: number = 0

  // Minimal profile (voltage) to use (updated on src caps load).
  // - 5500 mV when APDO available
  // - 5000 mV otherwise (first PDO)
  default_position: number = 1
  default_mv: number = ProfileSelector.DEFAULT_MV_FALLBACK

  load_pdos(pdos: POWER_DATA[]): void {
    this.descriptors = []

    for (const pdo of pdos) {
      const desc: PDO_DESCRIPTOR = {
        mv_min: pdo.mv_min < 5000 ? 5000 : pdo.mv_min,  // Clamp to ≥5V
        mv_max: pdo.mv_max,
        ma_max: pdo.ma_max,
        mohms_min: (pdo.mv_min * 1000) / pdo.ma_max,
        mohms_min_105_percent: 0,
        mohms_min_110_percent: 0,
      }
      desc.mohms_min_105_percent = desc.mohms_min * 105 / 100
      desc.mohms_min_110_percent = desc.mohms_min * 110 / 100

      this.descriptors.push(desc)
    }

    this.default_position = 1
    this.default_mv = ProfileSelector.DEFAULT_MV_FALLBACK

    // Try to find APDO with DEFAULT_MV_DESIRED support
    if (this.descriptors.length > 0) {
      for (let i = this.descriptors.length - 1; i > 0; i--) {
        const d = this.descriptors[i]
        if (d.mv_max >= ProfileSelector.DEFAULT_MV_DESIRED &&
            d.mv_min <= ProfileSelector.DEFAULT_MV_DESIRED) {
          this.default_position = i + 1
          this.default_mv = ProfileSelector.DEFAULT_MV_DESIRED
          break
        }
      }
    }

    this.set_pdo_index(0)
  }

  set_load_mohms(R_mohms: number): this {
    this.load_mohms = R_mohms
    return this
  }

  set_target_power_mw(power_mw: number): this {
    this.target_power_mw = power_mw
    return this
  }

  set_pdo_index(index: number): this {
    this.current_index = index
    this.better_index = index
    return this
  }

  mw_max(idx: number): number {
    const d = this.descriptors[idx]
    return Math.min(
      (d.mv_max * d.mv_max) / this.load_mohms,
      ((d.ma_max * d.ma_max / 1000) * this.load_mohms) / 1000
    )
  }

  mw_max_95_percent(idx: number): number {
    return this.mw_max(idx) * 95 / 100
  }

  mw_max_90_percent(idx: number): number {
    return this.mw_max(idx) * 90 / 100
  }

  private is_adjustable(desc: PDO_DESCRIPTOR): boolean {
    return desc.mv_min !== desc.mv_max
  }

  // Hysteresis and guards, to avoid oscillation and PWM noise:
  // - Upgrade when target > 95% of current Pmax (5% headroom left).
  // - Candidates must provide ≥10% headroom (power and current checks).
  // - Emergency downgrade when R < Rmin·1.05 (5% current margin).
  // - APDO guard: AVS often min at ~9 V. We clamp minV to 5 V overall;
  //   if APDO's minV is above 5 V and P(minV) > target·1.03, we'd need PWM
  //   and have no overcurrent trigger to step down — avoid/leave such APDO.
  // - Prefer APDO when possible; for FIXED try lower voltage if it still meets target.
  better_pdo_available(): boolean {
    if (this.descriptors.length === 0 || this.load_mohms === 0) {
      return false
    }

    let new_index = this.current_index

    // Safety check
    if (new_index >= this.descriptors.length) {
      new_index = 0
    }

    // Emergency case 1: we are close to overcurrent. Force downgrade to
    // first PDO (PD mandates 5 V FIXED at index 0), then continue with exact match.
    // 5% current margin (R < Rmin·1.05) to avoid tripping.
    if (this.load_mohms < this.descriptors[new_index].mohms_min_105_percent) {
      new_index = 0
    }

    // Emergency case 2: APDO/AVS minimal voltage too high for target
    {
      const d = this.descriptors[new_index]
      const min_continuous_mw = (d.mv_min * d.mv_min) / this.load_mohms
      if (this.is_adjustable(d) &&
          d.mv_min > 5000 &&
          min_continuous_mw > (this.target_power_mw * 103 / 100)) {
        new_index = 0
      }
    }

    // If desired power is close to PDO limit - try to upgrade
    // Upgrade when target exceeds 95% of current Pmax (5% headroom).
    if (this.target_power_mw > this.mw_max_95_percent(new_index)) {

      // First, try to find APDO. Satisfying 2 conditions:
      // - It can get desired power
      // - it not fall to PWM if min voltage > 5.0v
      for (let i = this.descriptors.length - 1; i >= 0; i--) {
        const d = this.descriptors[i]
        if (!this.is_adjustable(d)) continue
        if (this.load_mohms < d.mohms_min_110_percent) continue  // require 10% current margin

        // Avoid APDO that would require PWM at minV (>5 V)
        const min_continuous_mw = (d.mv_min * d.mv_min) / this.load_mohms
        if (d.mv_min > 5000 && min_continuous_mw > (this.target_power_mw * 103 / 100)) {
          continue
        }

        if (this.target_power_mw <= this.mw_max_90_percent(i)) {  // candidate has ≥10% headroom
          this.better_index = i
          return this.better_index !== this.current_index
        }
      }

      // If no APDO matched, try to find any PDO with higher power above target
      let current_max_mw = this.mw_max_90_percent(new_index)  // compare with 10% hysteresis

      for (let i = 0; i < this.descriptors.length; i++) {
        const d = this.descriptors[i]
        if (this.load_mohms < d.mohms_min_110_percent) continue  // require 10% current margin

        // Avoid APDO needing PWM at minV
        if (this.is_adjustable(d)) {
          const min_continuous_mw = (d.mv_min * d.mv_min) / this.load_mohms
          if (d.mv_min > 5000 && min_continuous_mw > (this.target_power_mw * 103 / 100)) {
            continue
          }
        }

        // Decline more weak profiles
        const mw_tmp = this.mw_max_90_percent(i)  // consider 10% headroom
        if (mw_tmp > current_max_mw) {
          new_index = i
          current_max_mw = mw_tmp
        }

        if (mw_tmp > this.target_power_mw) {  // candidate exceeds target with 10% headroom
          this.better_index = i
          return this.better_index !== this.current_index
        }
      }

      // If target power not satisfied, use the best we have
      this.better_index = new_index
      return this.better_index !== this.current_index
    }

    // For fixed profiles - try to downgrade to lower voltage if possible.
    if (!this.is_adjustable(this.descriptors[new_index])) {
      // First, try to find suitable APDO to avoid PWM.
      for (let i = 0; i < this.descriptors.length; i++) {
        const d = this.descriptors[i]
        if (!this.is_adjustable(d)) continue
        if (this.load_mohms < d.mohms_min_110_percent) continue  // require 10% current margin

        const mw_tmp = this.mw_max_90_percent(i)  // needs 10% power headroom
        if (mw_tmp < this.target_power_mw) continue

        // Avoid APDO needing PWM at minV (>5 V): 3% margin
        const min_continuous_mw = (d.mv_min * d.mv_min) / this.load_mohms
        if (d.mv_min > 5000 && min_continuous_mw > (this.target_power_mw * 103 / 100)) {
          continue
        }

        this.better_index = i
        return this.better_index !== this.current_index
      }

      // No suitable APDO, try to find lower fixed PDO
      for (let i = 0; i < this.descriptors.length; i++) {
        const d = this.descriptors[i]
        if (this.is_adjustable(d)) continue  // only FIXED
        if (this.load_mohms < d.mohms_min_110_percent) continue  // require 10% current margin
        if (d.mv_max >= this.descriptors[new_index].mv_max) continue  // only lower-voltage FIXED
        if (this.target_power_mw <= this.mw_max_90_percent(i)) {  // supports target with 10% headroom
          this.better_index = i
          return this.better_index !== this.current_index
        }
      }
    }

    // We can come here in 2 cases:
    // - no change needed
    // - emergencies updated new_index (e.g. to 0 → 5 V FIXED per PD),
    //   but no better profile available. Apply this properly.
    this.better_index = new_index
    return this.better_index !== this.current_index
  }

  get_pdo_usage_params(idx: number): PDO_USAGE_PARAMS {
    const params: PDO_USAGE_PARAMS = { mv: 0, duty_x1000: 0 }

    if (idx >= this.descriptors.length || this.load_mohms === 0) {
      return params
    }

    const d = this.descriptors[idx]

    // Calculate voltage to get target power or max available
    if (!this.is_adjustable(d)) {
      // FIXED PDO
      const mv = d.mv_min
      const max_mw = (mv * mv) / this.load_mohms

      if (this.target_power_mw > max_mw) {
        params.duty_x1000 = 1000
      } else {
        params.duty_x1000 = max_mw === 0 ? 0 : this.target_power_mw * 1000 / max_mw
      }

      params.mv = mv
      return params
    }

    // APDO (adjustable)
    // For APDO, calculate voltage to get target power
    // V = sqrt(P * R)
    const possible_mw = Math.min(this.target_power_mw, this.mw_max(idx))
    const ideal_mv = Math.sqrt(possible_mw * this.load_mohms)

    params.mv = Math.max(d.mv_min, Math.min(ideal_mv, d.mv_max))
    if (params.mv < this.default_mv) {
      params.mv = this.default_mv
    }

    const max_duty_mw = (params.mv * params.mv / this.load_mohms)

    if (max_duty_mw === 0) {
      params.duty_x1000 = 0
    } else {
      params.duty_x1000 = possible_mw * 1000 / max_duty_mw
    }

    // Add this to cut down on rounding errors
    if (params.duty_x1000 > 1000) {
      params.duty_x1000 = 1000
    }

    return params
  }
}
