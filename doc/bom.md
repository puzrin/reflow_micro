Bill of Materials
=================

Note: this BOM does NOT include MCH heater components. See the
[separate document](./mch_head/README.md) for details. It adds about $30 to
the cost.

### General

&nbsp; | Name | Comment
-------|------|--------
1 | [PCB & Components](https://oshwlab.com/puzrin/reflow-table-pd) | Open the EasyEDA project and order both in a couple of clicks. Order the PCB first and the components second so you can combine shipping.
2 | Location pins 1.0 or 0.6 mm | For SMT stencil positioning. [FDWQ-M1-D1-L10](https://jlcmc.com/product/s/F02/FDWQ/hexagonal-flange-face-self-tapping-drill-tail-screw?k=FDWQ-M1-D1-L10&productModelNumber=FDWQ-M1-D1-L10) / [AliExpress](https://www.aliexpress.com/item/1005007439330472.html). Any 1.0/0.6 mm wire will also work.
3 | M1.6 countersunk SS screws 16 mm (×4) | [EDLW-S1-M1.6-L16](https://jlcmc.com/product/s/E02/EDLW/gb-t-819-phillips-countersunk-head-screw?k=EDLW-S1-M1.6-L16&productModelNumber=EDLW-S1-M1.6-L16). Mount the heater plate to the reflector. Make sure they are stainless steel.
4 | M1.6 SS flat washers (×4) | [EPDA-S1W-B-1.6](https://jlcmc.com/product/s/E06/EPDA/flat-washer-level-a?k=EPDA-S1W-B-1.6&productModelNumber=EPDA-S1W-B-1.6). Hotplate mount.
5 | M1.6 SS spring lock washers (×8) | [EPDC-S1W-1.6](https://jlcmc.com/product/s/E06/EPDC/spring-washer-standard-spring-washers?k=EPDC-S1W-1.6&productModelNumber=EPDC-S1W-1.6). Hardware for the head mount.
6 | M1.6 SS nuts (×12) | [EMLA-S1W-BL1-M1.6](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M1.6&productModelNumber=EMLA-S1W-BL1-M1.6). Hardware for the head mount.
7 | M2 screws 3 mm (×14) | [EDDM-M2-L3](https://jlcmc.com/product/s/E02/EDDM/phillips-ultra-thin-head-screw?k=EDDM-M2-L3&productModelNumber=EDDM-M2-L3). Mount the reflector to the head base.
8 | M2 screws 10 mm (×10) | [EDLU-S1-M-M2-L10](https://jlcmc.com/product/s/E02/EDLU/gb-t-818-823-phillips-pan-head-screw?k=EDLU-S1-M-M2-L10&productModelNumber=EDLU-S1-M-M2-L10). Mount the fan and PCB.
9 | M2 nuts (×10) | [EMLA-S1W-BL1-M2](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M2&productModelNumber=EMLA-S1W-BL1-M2). Mount the fan and PCB.
10 | M3 countersunk hex screws 10 mm (×4) | [EDLJ-C16-M3-L10](https://jlcmc.com/product/s/E02/EDLJ/din-7991-gb-t-70.3-hex-socket-countersunk-head-screw) Black. Part of the cap's magnetic latch. The material should be magnetic; any non-stainless steel should work.
11 | [M2 4 mm insert nuts (×7)](https://www.aliexpress.com/item/1005008644449489.html) | Hardware for the head mount.
12 | [Magnets D6 L3 (×8)](https://www.aliexpress.com/item/1005010676875849.html) | Cap lock. N52 preferred, look for "magnets for 3d printers".
13 | [Magnets D4 L4 (×4)](https://www.aliexpress.com/item/1005010676875849.html) | Cap lock (double D4 L2 ok too). N52 preferred, look for "magnets for 3d printers".
14 | [Raspberry Pi 5 fan](https://he.aliexpress.com/item/1005006547048892.html) | Side-blowing fan for plate cooling.
15 | [AWG 26 wire](https://he.aliexpress.com/item/1005010779278423.html) | For MCPCB head only, power wires. Any 0.4 mm (0.126 mm²) tinned copper wire will be ok. The small cross-section helps reduce heat transfer.
16 | [Vinyl Electrical Tape 0.15 mm, black](https://www.aliexpress.com/item/1005010360658475.html) | For SLA prints only. Use it when installing M3 screws into the cap.

**LCSC order Notes.**

- Be sure to order components from both `main` and `head_base_XX` boards.
- If male connectors with boss pins (Liansheng FH-00147) not available, use any
  SMD 2mm 2x10P connectors with height 4.6 mm or below (body + pins).

### Optional

Treat this as a checklist; pick what you are missing from your bench.

&nbsp; | Name | Comment
-------|------|--------
1 | 140 W USB PD charger | PPS 5-21 V 5 A profile support is mandatory. A 28 V profile is required for the MCH heater (a 100 W charger is fine for the MCPCB heater). Single-port chargers are cheapest (25-30 USD). [Link 1](https://www.aliexpress.com/item/1005009049417279.html), [link 2](https://www.aliexpress.com/item/1005008926893357.html).
2 | [K-probe thermometer](https://www.aliexpress.com/item/1005008715777280.html) | For temperature calibration. The TM-902C from AliExpress is inexpensive and accurate enough.
3 | [Soldering air gun](https://www.aliexpress.com/item/1005006099512955.html) | For PCB assembly.
4 | Low-temp soldering paste | Sn42Bi58, 138°C, for PCB assembly. Standard leaded paste works too, but low-temp paste is more convenient.
5 | 217°C soldering paste | Use it for the PCB heater power lines.
6 | [Heat-set insert tip](https://www.aliexpress.com/item/1005009572892377.html) | For a nylon (PA-12) case only. Select the one that fits your iron.
7 | [Zero-flute countersink](https://www.aliexpress.com/item/1005004217081267.html) | Select the 1 mm size. For the MCPCB head only, countersink the MCPCB-based hotplate so the screw heads sit flush.


## JLCPCB / LCSC order notes

https://oshwlab.com/puzrin/reflow-table-pd

1. `main`: 1.6 mm, green (other colors for 4-layer PCBs are pricey).
   - Stencil is strongly recommended, it adds roughly $3 to the order and saves
     a ton of effort:
     - Bottom side only.
     - Select "Custom size" and set 90 x 80 mm (so the stencil stays compact and
       lightweight, keeping delivery costs down).
     - Select "Confirm production file" (ensure positional holes are properly generated)
     - Comment "**Мake marked blocks of holes (near top corners) on the stencil
       too. See doc layer for details.**". Otherwise, the positional holes might be missed.
2. `cap`: 1.6 mm, white.
3. `head_base_2R` / `head_base_7R`: 1.2 mm, white.
   - Use either for the MCPCB heater. For the MCH-based heater, choose the
     version that matches the MCH resistance. The difference is in the MCH
     connection: series or parallel.
4. `head_reflector`: 1.6 mm, white.
5. `mcpcb_heater`: aluminum, 1.6 mm, 1 oz.
   - Simple and cheap, but only up to 180°C.
   - For 250°C, see the [MCH heater](./mch_head/README.md) details (with the
     extra BOM).
6. `foil_conductor`: 1.0 mm, red.

jlc3dp prints:

1. `tray`, `cap_xxx`: SLA "JLC black resin" or MJF "PA12-HP nylon, black".
   - SLA: okay, budget price.
   - Nylon: about 2x more expensive than SLA, but better-looking.
   - (!) With nylon, use `cap_mjf_pa12` (reduced holes for self-tapping). For
     SLA use `cap_sla`.
2. `air_duct_pi5_top`, `air_duct_pi5_bottom`: MJF "PA12-HP nylon", unpainted (natural color).
3. `button`: SLA "8001 resin", translucent.
4. `tool_spacer`: SLA "9600 resin".
5. `pcb_aligner`: SLA "9600 resin".
6. `drill_conductor`: SLA "9600 resin" (for MCPCB heater only).
