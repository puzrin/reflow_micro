Heater assembly <!-- omit in toc -->
===============

- [Plate top painting](#plate-top-painting)
- [MCH heaters resistance matching](#mch-heaters-resistance-matching)
- [Mount heaters to plate](#mount-heaters-to-plate)
- [Mount reflector](#mount-reflector)
- [Mount base](#mount-base)
- [Bake paint](#bake-paint)


## Plate top painting

For good heat emission you heed to paint heating plate top to black color.
Difference with unpainted plate is very big. Note, high temperature paints need
baking at 160-200°C after dry. Read details in your paint instruction.

There are 2 approaches to baking:

1. If you have appropriate heater or regulated power source - bake immediately
   after paint dries. 
2. Bake after full device assembly, with built-in baking profile. In this case,
   you probably need waiting 1 day after painting for better dry, and be more
   careful to avoid scratches before plate been baked.

First, protect plate sides with sticky tape, and stick to A4 paper.

TBD image.

Clean plate top with IPA. Then put all on suitable surface and spay paint
from 20-30cm distance. Move sprayer to make thin and uniform layer. Small
thickness is important for strength on high temperatures.

TBD image.


## MCH heaters resistance matching

Since table uses 2 MCH heaters, those should have very close resistance for
balanced work. That's why you need buy heaters with some reserve.

1% tolerance will be good. There 2 ways to measure resistance equality.

**Variant 1**. Use milliohm meter. That requires a special device (see table of
optional purchases). If expences are not critical for you, or you plan to
experiment with MCH-s to customize plate - use this approach, and the most
convenient.

For exact result, Kelvin clips should be attached directly to MCH pads.

TBD image.

**Variant 2**. Use assembled PCB, with enabled debug info in settings. It will
show resistance then. You will have to temporary attach 1R 5W resistor with
terminal block for quick heaters swap. Resistor is required to limit current at
acceptable range.

Note, in this case MCH wires will add up to 10% of extra resistance. But,
since all MCH wires has the same length (add the same error), and we need to
know only MCH resistance deviation - this is ok.

TBD image.

Measure resistance of all you MCH heaters, and take 2 with the most close values.
1% tolerance is very good result. 2% - worst acceptable. 10 MCH-s should give
you 2-4 useable pairs.


## Mount heaters to plate

1. Apply thermal conductive paste to heaters, as small as posible.
2. Put heaters on plate, and slide +/-0.5mm for better paste spread.
3. Screw everything, using spring washers, with very small effort.

TBD image.

*Note. DON'T use big force on screws! Or MCH can crack after heating!*


## Mount reflector

TBD

## Mount base

Screw second PCB, using spring gaskets on the bottom. Then, cut and solder MCH
wires.

TBD image.

**Note 1**. Don't leave long wires. Those have big resistance, and will heat
too much.

**Note 2**. You may need active flux for MCH wires. Try "iron tip refresher".
It's important to use small amount and wash after.

Then, push pin terminals into main PCB for right positioning, place heater on
those, and solder.

TBD image.

Wash out flux after soldering.

## Bake paint

sdfs

