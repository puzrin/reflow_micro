// Ported from firmware/src/heater/profile_selector.hpp

import type { POWER_DATA } from './power_data'

export interface PDO_DESCRIPTOR {
  mv_min: number
  mv_max: number
  ma_max: number
  pdp_mw: number

  // Computed resistance limit
  mohms_min: number
}

export interface PDO_USAGE_PARAMS {
  mv: number
  duty_x1000: number
}

export class ProfileSelector {
  static readonly DEFAULT_MV_DESIRED = 5500
  static readonly DEFAULT_MV_FALLBACK = 5000

  private static readonly PROFILE_UP_SRC_POWER_THRESHOLD_PCT = 5
  private static readonly PROFILE_UP_DST_POWER_RESERVE_PCT = 10
  private static readonly PROFILE_DOWN_DST_POWER_RESERVE_PCT = 40
  private static readonly PROFILE_IN_CURRENT_MARGIN_PCT = 3
  private static readonly PROFILE_OUT_CURRENT_MARGIN_PCT = 1

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
      const mv_min = pdo.mv_min < 5000 ? 5000 : pdo.mv_min  // Clamp to >=5V
      const pdp_mw = (pdo.pdp_w ?? 0) * 1000
      let ma_max = pdo.ma_max
      if (ma_max <= 0 && pdp_mw > 0 && mv_min > 0) {
        ma_max = Math.floor((pdp_mw * 1000) / mv_min)
        if (ma_max > 5000) ma_max = 5000
      }
      const desc: PDO_DESCRIPTOR = {
        mv_min,
        mv_max: pdo.mv_max,
        ma_max,
        pdp_mw,
        mohms_min: ma_max > 0 ? (mv_min * 1000) / ma_max : Number.MAX_SAFE_INTEGER,
      }

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

  mw_max(idx: number, load_mohms: number = this.load_mohms): number {
    if (idx >= this.descriptors.length || load_mohms === 0) {
      return 0
    }
    const d = this.descriptors[idx]
    if (d.ma_max <= 0) {
      return 0
    }
    const mw_voltage = (d.mv_max * d.mv_max) / load_mohms
    const mw_current = ((d.ma_max * d.ma_max / 1000) * load_mohms) / 1000
    let mw = Math.min(mw_voltage, mw_current)
    if (d.pdp_mw > 0) {
      mw = Math.min(mw, d.pdp_mw)
    }
    return mw
  }

  private is_adjustable(desc: PDO_DESCRIPTOR): boolean {
    return desc.mv_min !== desc.mv_max
  }

  private is_descriptor_valid(idx: number): boolean {
    const d = this.descriptors[idx]
    return d.ma_max > 0 && d.mv_max > 0
  }

  private has_current_margin(idx: number, load_mohms: number, margin_pct: number): boolean {
    const required_mohms = this.descriptors[idx].mohms_min * (100 + margin_pct) / 100
    return load_mohms >= required_mohms
  }

  private mw_max_with_reserve(idx: number, load_mohms: number, reserve_pct: number): number {
    return this.mw_max(idx, load_mohms) * (100 - reserve_pct) / 100
  }

  private is_apdo_min_voltage_trap(idx: number, load_mohms: number, target_mw: number): boolean {
    const d = this.descriptors[idx]
    if (!this.is_adjustable(d)) return false
    if (d.mv_min <= this.default_mv) return false
    const min_continuous_mw = (d.mv_min * d.mv_min) / load_mohms
    return min_continuous_mw > (target_mw * 103 / 100)
  }

  // Select best PDO for target power.
  // - Emergency: overcurrent or APDO trap -> drop to PDO[0]
  // - Upgrade: when target > 95% of current max -> scan_up with 10% reserve
  // - Downgrade (FIXED only): when current profile has >40% excess -> scan_down
  better_pdo_available(): boolean {
    const load_mohms = this.load_mohms
    this.better_index = this.current_index

    if (this.descriptors.length === 0 || load_mohms === 0) {
      return false
    }

    let idx = this.current_index

    // Safety check
    if (idx >= this.descriptors.length || !this.is_descriptor_valid(idx)) {
      idx = 0
    }

    // Emergency case 1: close to overcurrent. Force downgrade to first PDO.
    if (!this.has_current_margin(idx, load_mohms, ProfileSelector.PROFILE_OUT_CURRENT_MARGIN_PCT)) {
      idx = 0
    }

    // Emergency case 2: APDO minimal voltage too high for target.
    if (this.is_apdo_min_voltage_trap(idx, load_mohms, this.target_power_mw)) {
      idx = 0
    }

    // Upgrade when target exceeds 95% of current Pmax (5% headroom).
    if (this.target_power_mw >
        this.mw_max_with_reserve(idx, load_mohms, ProfileSelector.PROFILE_UP_SRC_POWER_THRESHOLD_PCT)) {
      this.better_index = this.scan_up(idx, load_mohms, this.target_power_mw)
      return this.better_index !== this.current_index
    }

    // Downgrade: only for FIXED profiles
    if (!this.is_adjustable(this.descriptors[idx])) {
      const downgrade_candidate = this.scan_down(idx, load_mohms, this.target_power_mw)
      if (downgrade_candidate !== idx) {
        this.better_index = downgrade_candidate
        return this.better_index !== this.current_index
      }
    }

    this.better_index = idx
    return this.better_index !== this.current_index
  }

  private scan_up(from_idx: number, load_mohms: number, target_mw: number): number {
    // First pass: find suitable APDO (preferred to avoid PWM)
    for (let i = this.descriptors.length - 1; i >= 0; i--) {
      const d = this.descriptors[i]
      if (!this.is_adjustable(d)) continue
      if (!this.has_current_margin(i, load_mohms, ProfileSelector.PROFILE_IN_CURRENT_MARGIN_PCT)) continue
      if (this.is_apdo_min_voltage_trap(i, load_mohms, target_mw)) continue
      if (target_mw <=
          this.mw_max_with_reserve(i, load_mohms, ProfileSelector.PROFILE_UP_DST_POWER_RESERVE_PCT)) {
        return i
      }
    }

    // Second pass: find any PDO with enough power
    let best_idx = from_idx
    let best_mw = this.mw_max_with_reserve(
      from_idx,
      load_mohms,
      ProfileSelector.PROFILE_UP_DST_POWER_RESERVE_PCT
    )

    for (let i = 0; i < this.descriptors.length; i++) {
      if (!this.is_descriptor_valid(i)) continue
      if (!this.has_current_margin(i, load_mohms, ProfileSelector.PROFILE_IN_CURRENT_MARGIN_PCT)) continue
      if (this.is_apdo_min_voltage_trap(i, load_mohms, target_mw)) continue

      const mw_tmp = this.mw_max_with_reserve(i, load_mohms, ProfileSelector.PROFILE_UP_DST_POWER_RESERVE_PCT)
      if (mw_tmp > best_mw) {
        best_idx = i
        best_mw = mw_tmp
      }
      if (mw_tmp > target_mw) {
        return i
      }
    }

    return best_idx
  }

  private scan_down(from_idx: number, load_mohms: number, target_mw: number): number {
    // First pass: find suitable APDO (preferred to avoid PWM)
    for (let i = 0; i < this.descriptors.length; i++) {
      const d = this.descriptors[i]
      if (!this.is_adjustable(d)) continue
      if (!this.has_current_margin(i, load_mohms, ProfileSelector.PROFILE_IN_CURRENT_MARGIN_PCT)) continue
      if (this.is_apdo_min_voltage_trap(i, load_mohms, target_mw)) continue
      if (target_mw <=
          this.mw_max_with_reserve(i, load_mohms, ProfileSelector.PROFILE_DOWN_DST_POWER_RESERVE_PCT)) {
        return i
      }
    }

    // Second pass: find lower-voltage FIXED
    const current_mv = this.descriptors[from_idx].mv_max
    for (let i = 0; i < this.descriptors.length; i++) {
      const d = this.descriptors[i]
      if (this.is_adjustable(d)) continue
      if (!this.has_current_margin(i, load_mohms, ProfileSelector.PROFILE_IN_CURRENT_MARGIN_PCT)) continue
      if (d.mv_max >= current_mv) continue
      if (target_mw <=
          this.mw_max_with_reserve(i, load_mohms, ProfileSelector.PROFILE_DOWN_DST_POWER_RESERVE_PCT)) {
        return i
      }
    }

    return from_idx
  }

  get_pdo_usage_params(idx: number): PDO_USAGE_PARAMS {
    const params: PDO_USAGE_PARAMS = { mv: 0, duty_x1000: 0 }
    const load_mohms = this.load_mohms

    if (idx >= this.descriptors.length || load_mohms === 0 || !this.is_descriptor_valid(idx)) {
      return params
    }

    const d = this.descriptors[idx]

    // Calculate voltage to get target power or max available
    if (!this.is_adjustable(d)) {
      // FIXED PDO
      const mv = d.mv_min
      const max_mw = (mv * mv) / load_mohms
      const possible_mw = Math.min(this.target_power_mw, this.mw_max(idx, load_mohms))

      if (possible_mw > max_mw) {
        params.duty_x1000 = 1000
      } else {
        params.duty_x1000 = max_mw === 0 ? 0 : possible_mw * 1000 / max_mw
      }

      params.mv = mv
      return params
    }

    // APDO (adjustable)
    // For APDO, calculate voltage to get target power
    // V = sqrt(P * R)
    const possible_mw = Math.min(this.target_power_mw, this.mw_max(idx, load_mohms))
    const ideal_mv = Math.sqrt(possible_mw * load_mohms)

    params.mv = Math.max(d.mv_min, Math.min(ideal_mv, d.mv_max))
    if (params.mv < this.default_mv) {
      params.mv = this.default_mv
    }

    const max_duty_mw = (params.mv * params.mv / load_mohms)

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
