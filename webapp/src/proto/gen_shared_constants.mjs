import { readFileSync, writeFileSync } from 'node:fs'

const sourcePath = new URL('./types.proto', import.meta.url)
const tsOutputPath = new URL('../lib/shared_constants.ts', import.meta.url)
const hppOutputPath = new URL('../../../firmware/src/proto/generated/shared_constants.hpp', import.meta.url)

const proto = readFileSync(sourcePath, 'utf8')

const limitEntries = []
const enumEntries = []
const fieldPattern = /\[\s*((?:.|\n)*?)\s*\];/g
const enumMatch = /enum ConstantsBase\s*\{([\s\S]*?)\n\}/.exec(proto)

if (!enumMatch) {
  throw new Error('Failed to find enum ConstantsBase in types.proto')
}

for (const line of enumMatch[1].split('\n')) {
  const match = /^\s*([A-Z0-9_]+)\s*=\s*(-?\d+);/.exec(line)
  if (!match) continue

  enumEntries.push({
    name: match[1],
    value: Number(match[2]),
  })
}

for (const match of proto.matchAll(fieldPattern)) {
  const block = match[1]
  const exportName = /\(reflow_export_name\)\s*=\s*"([A-Z0-9_]+)"/.exec(block)
  const maxValue = /\(nanopb\)\.(?:max_count|max_length)\s*=\s*(\d+)/.exec(block)

  if (!exportName || !maxValue) continue

  limitEntries.push({
    name: exportName[1],
    value: Number(maxValue[1]),
  })
}

const tsMembers = [...enumEntries, ...limitEntries]

const tsFile = `// Auto-generated file. DO NOT EDIT.
export const SharedConstants = {
${tsMembers.map(({ name, value }) => `  ${name}: ${value},`).join('\n')}
}
`

const hppFile = `// Auto-generated file. DO NOT EDIT.
#pragma once

namespace SharedConstants {
${tsMembers.map(({ name, value }) => `  inline constexpr int ${name} = ${value};`).join('\n')}
} // namespace SharedConstants
`

writeFileSync(tsOutputPath, tsFile)
writeFileSync(hppOutputPath, hppFile)
