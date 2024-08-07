#! /usr/bin/env node

import { execSync } from 'node:child_process'
import { argv } from 'node:process'

const executables = ['openscad-nightly', 'openscad']

let openscad = ''

for (let name of executables) {
  try {
    execSync(`which ${name}`)
    openscad = name
    break
  } catch {}
}

if (!openscad) {
  console.error('No OpenSCAD executable found')
  process.exit(1)
}

const p = (name) => new URL(name, import.meta.url).pathname

const builds = {
  tray: ['case.scad', '-D DRAW_TRAY=1'],
  cap: ['case.scad', '-D DRAW_CAP=1'],
  cap_alt: ['case.scad', '-D DRAW_CAP=1 -D magnet_h=6.5'],
  button: ['case.scad', '-D DRAW_BTN=1'],
  button_03: ['case.scad', '-D DRAW_BTN=1 -D btn_h=2.7 -D btn_w=7.7'],
  button_04: ['case.scad', '-D DRAW_BTN=1 -D btn_h=2.6 -D btn_w=7.6'],
  button_05: ['case.scad', '-D DRAW_BTN=1 -D btn_h=2.5 -D btn_w=7.5'],
  // air_duct_10_5: ['air_duct.scad', '-D h=10.5 -D top_dia=10'],
  // air_duct_alt_10_5: ['air_duct.scad', '-D h=10.5 -D top_dia=15'],
  // air_duct_9_5: ['air_duct.scad', '-D h=9.5 -D top_dia=10'],
  // air_duct_alt_9_5: ['air_duct.scad', '-D h=9.5 -D top_dia=15'],
  air_duct_pi5_top: ['air_duct_pi5.scad', '-D draw_top=1'],
  air_duct_pi5_bottom: ['air_duct_pi5.scad', '-D draw_bottom=1'],
  pcb_aligner_03: ['pcb_aligner.scad', '-D latch=0.3'],
  pcb_aligner_05: ['pcb_aligner.scad', '-D latch=0.5'],
  magnet_clamp_80: ['magnet_clamp.scad', '-D space=8'],
  magnet_clamp_75: ['magnet_clamp.scad', '-D space=7.5'],
  magnet_clamp_70: ['magnet_clamp.scad', '-D space=7.0'],
  tool_spacer: ['tool_spacer.scad', ''],
}

function build(name) {
  const [file, options] = builds[name]
  console.log(`Building ${name}.stl`)
  execSync(`${openscad} ${p(file)} ${options} -o ${p(`../${name}.stl`)}`)
}

// no params - build all variants
if (argv.length === 2) {
  Object.keys(builds).forEach(build)
  process.exit(0)
}

//
// build specified variants
//

const variants = argv.slice(2)

// Make sure all requested variants exist
for (const variant of variants) {
  if (!Object.keys(builds).includes(variant)) {
    console.error(`Unknown variant: ${variant}`)
    process.exit(1)
  }
}

for (const variant of variants) build(variant)
