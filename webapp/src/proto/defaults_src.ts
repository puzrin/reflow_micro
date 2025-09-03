// Used to generate protobuf data binaries. Don't use directly.

import { ProfilesData, HeadParams } from './generated/types'

// https://www.compuphase.com/electronics/reflowsolderprofiles.htm

export const profiles_default: ProfilesData = {
  selectedId: 1,
  items: [
    {
      id: 1,
      name: 'Reflow Leaded',
      segments: [
        { target: 150, duration: 60 },
        { target: 165, duration: 90 },
        { target: 235, duration: 30 },
        { target: 235, duration: 20 },
        { target: 40, duration: 50}
      ]
    },
    {
      id: 2,
      name: 'Reflow Lead Free',
      segments: [
        { target: 150, duration: 60 },
        { target: 180, duration: 120 },
        { target: 255, duration: 30 },
        { target: 255, duration: 15 },
        { target: 40, duration: 50}
      ]
    },
    {
      id: 3,
      name: 'Reflow LTS',
      segments: [
        { target: 100, duration: 40 },
        { target: 120, duration: 120 },
        { target: 190, duration: 30 },
        { target: 190, duration: 20 },
        { target: 40, duration: 40}
      ]
    },
    {
      id: 4,
      name: 'Preheat Lead Free / Leaded',
      segments: [
        { target: 150, duration: 60 },
        { target: 150, duration: 3600 }
      ]
    },
    {
      id: 5,
      name: 'Preheat LTS',
      segments: [
        { target: 120, duration: 50 },
        { target: 120, duration: 3600 }
      ]
    },
    {
      id: 6,
      name: 'Paint bake',
      segments: [
        { target: 180, duration: 6600 },
        { target: 250, duration: 86400 }
      ]
    }
  ]
}

export const head_default: HeadParams = {
  sensor_p0_temperature: -1000,
  sensor_p0_value: 0,
  sensor_p1_temperature: -1000,
  sensor_p1_value: 0,
  adrc_response: 132,
  adrc_b0: 0.0202,
  adrc_N: 50,
  adrc_M: 3
}
