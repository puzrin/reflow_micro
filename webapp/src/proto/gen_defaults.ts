/// <reference types="node" />

import { ProfilesData, HeadParams, DeviceState } from './generated/types'
import { profiles_default, head_default } from './defaults_src';
import { writeFileSync } from 'fs'

const ts_header = '// Auto-generated file. DO NOT EDIT.'
const hpp_header = `// Auto-generated file. DO NOT EDIT.
#pragma once

#include <cstdint>`

function write_relative(filepath: string, data: string): void {
  writeFileSync(new URL(filepath, import.meta.url), data)
}

function toHexBlock(bytes: Uint8Array, indent: number = 0): string {
  const hex = Buffer.from(bytes).toString('hex')
    .match(/.{1,16}/g)
    ?.map((line: string) =>
      ' '.repeat(indent) + line.match(/../g)?.map(b => '0x' + b).join(', ')
    ).join(',\n');

  return hex ?? ''
}


function to_ts(export_name: string, data: Uint8Array): string {
  return `export const ${export_name} = new Uint8Array([
${toHexBlock(data, 2)}
])`
}

function to_hpp(export_name: string, data: Uint8Array): string {
  return `inline const uint8_t ${export_name}[] = {
${toHexBlock(data, 4)}
};`
}

write_relative('./generated/defaults.ts', [
  ts_header,
  to_ts('DEFAULT_PROFILES_DATA_PB', ProfilesData.encode(profiles_default).finish()),
  to_ts('DEFAULT_HEAD_PARAMS_PB', HeadParams.encode(head_default).finish())
].join('\n\n') + `\n`)

const profiles_default_unselected = { ...profiles_default, selectedId: -1 }

write_relative('../../../firmware/src/proto/generated/defaults.hpp', [
  hpp_header,
  //to_hpp('DEFAULT_PROFILES_DATA_PB', ProfilesData.encode(profiles_default).finish()),
  // Split into selection & unselected data tp simplify initialization & updates
  `inline const int32_t DEFAULT_PROFILES_SELECTION = ${profiles_default.selectedId};`,
  to_hpp('DEFAULT_PROFILES_DATA_UNSELECTED_PB', ProfilesData.encode(profiles_default_unselected).finish()),
  to_hpp('DEFAULT_HEAD_PARAMS_PB', HeadParams.encode(head_default).finish())
].join('\n\n') + `\n`)

// Validate DeviceState fields, required for FSM
const state_values = Object.values(DeviceState).filter(v => typeof v === 'number' && v >= 0);
if (!state_values.every((v, i) => v === i)) {
  throw new Error('DeviceState values must be sequential starting from 0');
}

