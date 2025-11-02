import { ProfileSelector } from './profile_selector'
import type { Head } from './head'
import type { POWER_DATA } from './power_data'

function interpolate(x: number, points: [number, number][]): number {
  if (points.length < 2) {
    throw new Error("At least two points are required for interpolation.")
  }

  if (x <= points[0][0]) {
    const [x1, y1] = points[0]
    const [x2, y2] = points[1]
    return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
  }

  if (x >= points[points.length - 1][0]) {
    const [x1, y1] = points[points.length - 2]
    const [x2, y2] = points[points.length - 1]
    return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
  }

  for (let i = 1; i < points.length; i++) {
    if (x < points[i][0]) {
      const [x1, y1] = points[i - 1]
      const [x2, y2] = points[i]
      return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
    }
  }

  throw new Error("Interpolation error: x is out of bounds.")
}

export class Power {
  selector = new ProfileSelector()
  head: Head

  peak_mv: number = 0
  peak_ma: number = 0
  duty_x1000: number = 0

  constructor(head: Head) {
    this.head = head
  }

  setup(pdos: POWER_DATA[]): this {
    this.selector.load_pdos(pdos)
    this.selector.set_pdo_index(0)
    return this
  }

  set_power_mw(mw: number): void {
    this.selector.set_target_power_mw(mw)
  }

  // Calculate load resistance from current temperature (replacement for PWM measurement)
  get_load_mohm(): number {
    const temp = this.head.temperature
    const points = this.head.calibration_points

    if (points.length === 0) {
      throw new Error("No calibration points defined.")
    }

    if (points.length === 1) {
      // Single point: use Tungsten TC
      const TUNGSTEN_TC = 0.0041
      const single_point = points[0]
      return single_point.R * (1 + TUNGSTEN_TC * (temp - single_point.T)) * 1000  // → mOhm
    }

    // Multi-point: interpolate T → R
    const t_r_points = points.map(p => [p.T, p.R] as [number, number])
    const resistance_ohms = interpolate(temp, t_r_points)
    return resistance_ohms * 1000  // → mOhm
  }

  tick(): void {
    const R_mohms = this.get_load_mohm()

    this.selector.set_load_mohms(R_mohms)

    // Check if better PDO available and switch
    if (this.selector.better_pdo_available()) {
      this.selector.set_pdo_index(this.selector.better_index)
    }

    // Get PDO usage parameters
    const params = this.selector.get_pdo_usage_params(this.selector.current_index)
    this.peak_mv = params.mv
    this.duty_x1000 = params.duty_x1000

    // Calculate actual peak current from Ohm's law: I = V / R
    // peak_mv (mV) * 1000 / R_mohms (mΩ) = mA
    this.peak_ma = this.peak_mv * 1000 / R_mohms
  }

  get_max_power_mw(): number {
    return this.selector.mw_max(this.selector.current_index)
  }
}
