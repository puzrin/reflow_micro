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
  'tray_insert_3.25': ['case.scad', '-D DRAW_TRAY=1 -D insert_d=3.25'],
  cap: ['case.scad', '-D DRAW_CAP=1'],
  button: ['case.scad', '-D DRAW_BTN=1'],
  air_duct_pi5_top_10: ['air_duct_pi5.scad', '-D draw_top=1'],
  air_duct_pi5_top_11: ['air_duct_pi5.scad', '-D draw_top=1 -D top_h=11'],
  air_duct_pi5_top_straight_10: ['air_duct_pi5.scad', '-D draw_top=1 -D conic=0'],
  air_duct_pi5_top_straight_11: ['air_duct_pi5.scad', '-D draw_top=1 -D top_h=11 -D conic=0'],
  air_duct_pi5_bottom: ['air_duct_pi5.scad', '-D draw_bottom=1'],
  pcb_aligner: ['pcb_aligner.scad', '-D latch=0.3'],
  magnet_clamp: ['magnet_clamp.scad', '-D space=7.5'],
  tool_spacer: ['tool_spacer.scad', ''],
  ss_mch_mount: ['ss_mch_mount.scad', ''],
  drill_conductor: ['drill_conductor.scad', '']
}

function build(name) {
  const [file, options] = builds[name]
  console.log(`Building ${name}.stl`)
  execSync(`${openscad} ${p(file)} --export-format binstl --enable predictible-output ${options} -o ${p(`../${name}.stl`)}`)
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
