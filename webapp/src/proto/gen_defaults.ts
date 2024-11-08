/// <reference types="node" />

import { ProfilesData } from './generated/types'
import { writeFileSync} from 'fs'

function toHexBlock(bytes: Uint8Array, indent: number = 0): string {
  const hex = Buffer.from(bytes).toString('hex')
    .match(/.{1,16}/g)
    ?.map((line: string) =>
      ' '.repeat(indent) + line.match(/../g)?.map(b => '0x' + b).join(', ')
    ).join(',\n');

  return hex ?? ''
}

function to_ts(filepath: string, export_name: string, data: Uint8Array): void {
  const ts_template = `// Auto-generated file. DO NOT EDIT.
export const ${export_name} = new Uint8Array([
${toHexBlock(data, 2)}
])
`
  writeFileSync(new URL(filepath, import.meta.url), ts_template)
}

function to_hpp(filepath: string, export_name: string, data: Uint8Array): void {
  const hpp_template = `// Auto-generated file. DO NOT EDIT.
#pragma once

#include <cstdint>

inline uint8_t ${export_name}[] = {
${toHexBlock(data, 4)}
};
`
  writeFileSync(new URL(filepath, import.meta.url), hpp_template)
}

// https://www.compuphase.com/electronics/reflowsolderprofiles.htm

const defaultProfilesData: ProfilesData = {
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
        { target: 180, duration: 4500 }
      ]
    }
  ]
}

const bin = ProfilesData.encode(defaultProfilesData).finish()
to_ts('./generated/profiles_data_pb.ts', 'DEFAULT_PROFILES_DATA_PB', bin)
to_hpp('./generated/profiles_data_pb.hpp', 'DEFAULT_PROFILES_DATA_PB', bin)
