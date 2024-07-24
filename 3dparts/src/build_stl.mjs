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
  button: ['case.scad', '-D DRAW_BTN=1'],
  tray: ['case.scad', '-D DRAW_TRAY=1'],
  cap: ['case.scad', '-D DRAW_CAP=1'],
  air_duct: ['air_duct.scad', '-D top_dia=10'],
  air_duct_alt: ['air_duct.scad', '-D top_dia=15 -D bottom_dia=15'],
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
