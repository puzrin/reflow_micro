export type HEATER_CALIBRATION_POINT = {
  T: number
  W: number
  R: number
}

export type HEATER_DATA = {
  size: {
    x: number
    y: number
    z: number
  }
  // Correction to compensate MCH volume and other things, to make modeled
  // response time more close to real devices.
  // default is 1 (no correction)
  heat_capacity_scale?: number
  calibration_points: HEATER_CALIBRATION_POINT[]
}

// MCH, black top, 70x80x3mm
export const mch_black_70x80x3: HEATER_DATA = {
  size: { x: 0.08, y: 0.07, z: 0.0028 },
  heat_capacity_scale: 1.35,
  calibration_points: [
    { T: 21.0, W: 0.0, R: 3.775 },
    { T: 85.7, W: 9.7, R: 4.728 },
    { T: 109.0, W: 14.3, R: 5.084 },
    { T: 131.2, W: 19.3, R: 5.436 },
    { T: 150.6, W: 24.4, R: 5.734 },
    { T: 168.5, W: 28.9, R: 6.015 },
    { T: 188.0, W: 34.0, R: 6.322 },
    { T: 203.2, W: 39.0, R: 6.566 },
    { T: 218.9, W: 43.8, R: 6.834 },
    { T: 233.1, W: 48.8, R: 7.605 },
    { T: 268.8, W: 62.2, R: 7.637 }
  ]
}
