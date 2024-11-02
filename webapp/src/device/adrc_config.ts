export interface AdrcConfig {
  // System response time (when temperature reaches 63% of final value)
  response: number
  // Scale. Max derivative / power
  b0: number
  // ω_observer = N / τ. Usually 3..10
  // 3 is good for the start. Increase until oscillates, then back 10-20%.
  N: number
  // ω_controller = ω_observer / M. Usually 2..5
  // 3 is a good for the start. Probably, changes not required.
  M: number

  //
  // Metadata
  //

  // Power of step test signal
  W?: number
}

export const defaultAdrcConfig: AdrcConfig = {
  response: 120,
  b0: 0.0263,
  N: 20,
  M: 3,
  W: 50
}