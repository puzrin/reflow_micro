export interface AdrcConfig {
  // System response time (when temperature reaches 63% of final value)
  response: number
  // Scale. Max derivative / power
  b0: number
  // ω_observer = N / τ. Usually 3..10
  // 5 is good for the start. Increase until oscillates, then back 10-20%.
  N: number
  // ω_controller = ω_observer / M. Usually 2..5
  // 3 is a good for the start. Probably, changes not required.
  M: number
}

export const defaultAdrcConfig: AdrcConfig = {
  response: 132,
  b0: 0.0202,
  N: 50,
  M: 3
}