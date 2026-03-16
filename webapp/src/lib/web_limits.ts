export const PROFILE_LIMITS = {
  targetMin: 0,
  targetMax: 300,
  durationMin: 5,
  durationMax: 60 * 60 * 24,
  nameMinChars: 3,
} as const

export const SENSOR_CALIBRATION_LIMITS = {
  point0Min: 0,
  point0Max: 50,
  point0Step: 0.1,
  bakePowerMin: 1,
  bakePowerMax: 100,
  point1Min: 100,
  point1Max: 300,
  point1Step: 0.1,
} as const

export const ADRC_LIMITS = {
  tauMin: 1,
  tauMax: 1000,
  b0Min: 0.00001,
  b0Max: 1,
  b0Step: 0.00001,
  nMin: 3,
  nMax: 500,
  nStep: 0.5,
  mMin: 2,
  mMax: 10,
  mStep: 0.5,
  stepResponsePowerMin: 1,
  stepResponsePowerMax: 100,
  testTemperatureMin: 0,
  testTemperatureMax: 300,
} as const
