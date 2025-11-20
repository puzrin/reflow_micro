// Used to generate protobuf data binaries. Don't use directly.

import { ProfilesData, HeadParams } from './generated/types'

// https://www.compuphase.com/electronics/reflowsolderprofiles.htm

export const profiles_default: ProfilesData = {
  selectedId: 1,
  items: [
    {
      id: 1,
      name: 'Reflow LTS',
      segments: [
        { target: 100, duration: 60 },
        { target: 140, duration: 60 },
        { target: 180, duration: 30 },
        { target: 180, duration: 30 },
        { target: 140, duration: 20 }
      ]
    },
    {
      id: 2,
      name: 'Reflow Leaded',
      segments: [
        { target: 150, duration: 120 },
        { target: 170, duration: 60 },
        { target: 230, duration: 60 },
        { target: 230, duration: 30 },
        { target: 170, duration: 50}
      ]
    },
    {
      id: 3,
      name: 'Preheat 150°C',
      segments: [
        { target: 150, duration: 60 },
        { target: 150, duration: 3600 }
      ]
    }
  ]
}

export const head_default: HeadParams = {
  sensor_p0_at: -1000,
  sensor_p0_value: 0,
  sensor_p1_at: -1000,
  sensor_p1_value: 0,
  adrc_response: 113,
  adrc_b0: 0.0536,
  adrc_N: 55,
  adrc_M: 5
}
