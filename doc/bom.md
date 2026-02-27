Bill of Materials
=================

Note, this BOM does NOT include MCH heater components. See
[separate document](./mch_head/README.md) for details. It will edd extra 30$
cost.

### General

&nbsp; | Name | Comment
-------|------|--------
1 | [PCB & Components](https://oshwlab.com/puzrin/reflow-table-pd) | Open the EasyEDA project and order both in a couple of clicks. Order the PCB first, components second so you can combine shipping.
2 | Locatin pins 1.0 or 0.6 mm | For SMT stencil positioning. [FDWQ-M1-D1-L10](https://jlcmc.com/product/s/F02/FDWQ/hexagonal-flange-face-self-tapping-drill-tail-screw?k=FDWQ-M1-D1-L10&productModelNumber=FDWQ-M1-D1-L10) / [AliExpress](https://www.aliexpress.com/item/1005007439330472.html). Also any 1.0/0.6 mm wire is suitable.
3 | M1.6 countersunk SS screws 16 mm (×4) | [EDLW-S1-M1.6-L16](https://jlcmc.com/product/s/E02/EDLW/gb-t-819-phillips-countersunk-head-screw?k=EDLW-S1-M1.6-L16&productModelNumber=EDLW-S1-M1.6-L16). Mount the heater plate to the reflector. Make sure they are stainless steel.
4 | M1.6 SS spring lock washers (×8) | [EPDC-S1W-1.6](https://jlcmc.com/product/s/E06/EPDC/spring-washer-standard-spring-washers?k=EPDC-S1W-1.6&productModelNumber=EPDC-S1W-1.6). Hardware for the head mount.
5 | M1.6 SS nuts (×12) | [EMLA-S1W-BL1-M1.6](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M1.6&productModelNumber=EMLA-S1W-BL1-M1.6). Hardware for the head mount.
6 | M2 screws 3 mm (×14) | [EDDM-M2-L3](https://jlcmc.com/product/s/E02/EDDM/phillips-ultra-thin-head-screw?k=EDDM-M2-L3&productModelNumber=EDDM-M2-L3). Mount the reflector to the head base.
7 | M2 screws 10 mm (×10) | [EDLU-S1-M-M2-L10](https://jlcmc.com/product/s/E02/EDLU/gb-t-818-823-phillips-pan-head-screw?k=EDLU-S1-M-M2-L10&productModelNumber=EDLU-S1-M-M2-L10). Mount the fan and PCB.
8 | M2 nuts (×10) | [EMLA-S1W-BL1-M2](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M2&productModelNumber=EMLA-S1W-BL1-M2). Mount the fan and PCB.
9 | M3 countersunk hex screws 10 mm (×4) | [EDLJ-C16-M3-L10](https://jlcmc.com/product/s/E02/EDLJ/din-7991-gb-t-70.3-hex-socket-countersunk-head-screw) Black. Parts of cap magnetic lock. Material should be magnetic, any NOT stainless steel will be ok.
10 | [M2 4 mm insert nuts (×7)](https://www.aliexpress.com/item/1005008644449489.html) | Hardware for the head mount.
11 | [Magnets D6 L3 (×8)](https://www.aliexpress.com/item/1005010676875849.html) | Cap lock. N52 preferred, look for "magnets for 3d printers".
12 | [Magnets D4 L4 (×4)](https://www.aliexpress.com/item/1005010676875849.html) | Cap lock (double D4 L2 ok too). N52 preferred, look for "magnets for 3d printers".
13 | [0.4 mm bare copper wire](https://www.aliexpress.com/item/1005009540818990.html) | For MCPCB head only, power wires. The small cross-section helps reduce heat transfer.
14 | [Vinyl Electrical Tape 0.15 mm, black](https://www.aliexpress.com/item/1005010360658475.html) | For SLA print only. Use to install M3 screws into cap

**LCSC order Notes.**

- Ensure to order components from both `main` and `head_base_XX` boards.
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
7 | [Zero-flute countersink](https://www.aliexpress.com/item/1005004217081267.html) | Select the 1 mm size.  For MCPCB head only, countersink the MCPCB-based hotplate so the screw heads sit flush.


## JLCPCB / LCSC order notes

https://oshwlab.com/puzrin/reflow-table-pd

1. `main`: 1.6 mm, green (other colors for 4-layer PCBs are pricey).
   - Stencil is strongly recommended, it adds roughly $3 to the order and saves
     a ton of effort:
     - Bottom side only.
     - Comment "**Make stencil according to paste mask file and don't forget the corner holes marked with arrows**". Otherwise, the positional holes might be missed.
     - Select "Custom size" and set 90 x 80 mm so the stencil stays compact and
       lightweight, keeping delivery costs down.
     - Select "Confirm production file" (ensure positional holes are properly generated)
2. `cap`: 1.6 mm, white.
3. `head_base_2R` / `head_base_7R`: 1.2 mm, white.
   - Use any for MCPCB heater. For MCH-based heater use appropriate to MCH
     resistance. The difference is in MCH connection - consecutive or parallel.
4. `head_reflector`: 1.6 mm, white.
5. `mcpcb_heater`: aluminum, 1.6 mm, 1 oz.
   - Simple and cheap, but only up to 180°C
   - For 250°C see [MCH heater](./mch_head/README.md) detils (with extra BOM).
6. `foil_conductor`: 1.0 mm, red.

jlc3dp prints:

1. `tray`, `cap_xxx`: SLA "JLC black resin" or MJF "PA12-HP nylon, black".
   - SLA: okay, budget price.
   - Nylon: 2x more expensive than SLA, better-looking.
   - (!) With nylon, use `cap_mjf_pa12` (reduced holes for self-tapping). For
     SLA use `cap_sla`.
2. `air_duct_pi5_top`, `air_duct_pi5_bottom`: MJF "PA12-HP nylon", unpainted (natural color).
3. `button`: SLA "8001 resin", translucent.
4. `tool_spacer`: SLA "9600 resin".
5. `pcb_aligner`: SLA "9600 resin".
6. `drill_conductor`: SLA "9600 resin" (for MCPCB heater only).
