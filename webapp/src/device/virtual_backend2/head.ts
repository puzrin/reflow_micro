import type { HEATER_DATA } from './heater_data'

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

export class Head {
  size: { x: number; y: number; z: number }
  calibration_points: { T: number; R: number; W: number }[]
  temperature: number
  heat_capacity_scale = 1

  constructor(x = 0.08, y = 0.07, z = 0.0038) {
    this.size = { x, y, z } // Plate dimensions in meters
    this.calibration_points = [] // Calibration points for resistance and heat dissipation
    this.temperature = 25 // Default room temperature
  }

  clone(): Head {
    const new_instance = new Head(this.size.x, this.size.y, this.size.z)
    new_instance.calibration_points = this.calibration_points.map((point) => ({ ...point }))
    new_instance.temperature = this.temperature
    new_instance.heat_capacity_scale = this.heat_capacity_scale
    return new_instance
  }

  reset(): this {
    this.temperature = this.get_room_temp()
    return this
  }

  setup(config: HEATER_DATA): this {
    this.set_size(config.size.x, config.size.y, config.size.z)
    this.heat_capacity_scale = config.heat_capacity_scale ?? 1
    for (const point of config.calibration_points) {
      this.calibrate(point.T, point.W, point.R)
    }
    this.reset()
    return this
  }

  set_size(x: number, y: number, z: number): this {
    this.size = { x, y, z }
    return this
  }

  scale_r_to(new_base: number): this {
    // Scale the resistance in all calibration points
    const ratio = new_base / this.calibration_points[0].R
    for (const point of this.calibration_points) {
      point.R *= ratio
    }
    return this
  }

  calibrate(T: number, W: number, R: number): this {
    this.calibration_points.push({ T, W, R })

    // Set initial temperature on first calibration point
    if (this.calibration_points.length === 1) {
      this.temperature = T
    }

    // Sort calibration points by temperature
    this.calibration_points.sort((a, b) => a.T - b.T)

    // Minimal validation
    for (let i = 1; i < this.calibration_points.length; i++) {
      if (this.calibration_points[i].R < this.calibration_points[i - 1].R) {
        throw new Error("Calibration points must have non-decreasing resistance values.")
      }
    }

    return this
  }


  calculate_heat_capacity(): number {
    const material_shc = 897 // J/kg/K for Aluminum 6061
    const material_density = 2700 // kg/m3 for Aluminum 6061
    const volume = this.size.x * this.size.y * this.size.z // in m³
    const mass = volume * material_density // in kg
    return this.heat_capacity_scale * mass * material_shc
  }

  calculate_heat_transfer_coefficient(): number {
    const points_with_power = this.calibration_points.filter((p) => p.W !== 0)

    if (!points_with_power.length) {
      const area = this.size.x * this.size.y // in m²
      return 40 * area // in W/K, Default empiric value
    }

    if (points_with_power.length === 1) {
      const point = points_with_power[0]
      return point.W / (point.T - this.get_room_temp())
    }

    const points = points_with_power.map(
      (p) => [p.T, p.W / (p.T - this.get_room_temp())] as [number, number]
    )
    return interpolate(this.temperature, points)
  }

  get_room_temp(): number {
    const room_temp_point = this.calibration_points.find((p) => p.W === 0)
    return room_temp_point ? room_temp_point.T : 25
  }

  iterate(power_watts: number, dt_seconds: number): void {
    const heat_capacity = this.calculate_heat_capacity()
    const heat_transfer_coefficient = this.calculate_heat_transfer_coefficient()
    const temperature_change =
      ((power_watts - heat_transfer_coefficient * (this.temperature - this.get_room_temp())) *
        dt_seconds) /
      heat_capacity

    this.temperature += temperature_change
  }
}
