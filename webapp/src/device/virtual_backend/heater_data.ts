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

// MCPCB, 80x70x1.6mm
export const mcpcb_80x70x16: HEATER_DATA = {
  size: { x: 0.08, y: 0.07, z: 0.0016 },
  heat_capacity_scale: 1,
  calibration_points: [
    { T: 23.0, W: 0.0, R: 3.061 },
    { T: 59.8, W: 4.64, R: 3.502 },
    { T: 90.1, W: 9.45, R: 3.861 },
    { T: 118.8, W: 14.09, R: 4.202 },
    { T: 145.1, W: 19.03, R: 4.506 },
    { T: 167.4, W: 23.76, R: 4.763 }
  ]
}

// MCH, 80x70x2.8mm
export const mch_80x70x3: HEATER_DATA = {
  size: { x: 0.08, y: 0.07, z: 0.0028 },
  heat_capacity_scale: 1.35,
  calibration_points: [
    { T:  21.3, W:   0.0, R: 3.790 },
    { T:  89.2, W:  9.59, R: 4.795 },
    { T: 117.1, W: 14.29, R: 5.210 },
    { T: 142.3, W: 19.30, R: 5.604 },
    { T: 164.2, W: 24.00, R: 5.944 },
    { T: 185.3, W: 28.96, R: 6.280 },
    { T: 206.4, W: 33.78, R: 6.639 },
    { T: 224.3, W: 38.86, R: 6.934 },
    { T: 244.4, W: 43.56, R: 7.250 },
    { T: 256.2, W: 48.37, R: 7.451 },
    { T: 286.0, W: 57.27, R: 8.014 }
  ]
}

// MCH, 80x70x2.8mm, black top (discontinued)
export const mch_black_80x70x3: HEATER_DATA = {
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
    //{ T: 233.1, W: 48.8, R: 7.605 }, // bad R value
    { T: 268.8, W: 62.2, R: 7.637 }
  ]
}
