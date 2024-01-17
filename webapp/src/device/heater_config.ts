export interface Segment {
  target: number
  duration: number
}

export interface Profile {
  id: number
  name: string
  segments: Segment[]
}

// https://www.compuphase.com/electronics/reflowsolderprofiles.htm

const defaultProfiles: Profile[] = [
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
      { target: 180, duration: 4500 }
    ]
  }
]

export interface ProfilesStoreData {
  items: Profile[]
  selectedId: number
}

export const defaultProfilesStoreData: ProfilesStoreData = {
  items: defaultProfiles,
  selectedId: 1
}

export function createDummyProfile(): Profile {
  return {
    id: 0,
    name: '',
    segments: [
      { target: 150, duration: 60 }
    ]
  }
}

export const startTemperature = 30
export const coolDownTemperature = 50
export const limits = {
  targetMin: 30,
  targetMax: 300,
  durationMin: 5,
  durationMax: 60*60*24,
  nameMin: 3,
  nameMax: 30
}
