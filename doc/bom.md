Bill of Materials
=================

### General

&nbsp; | Name | Comment
-------|------|--------
1 | [PCB & Components](https://oshwlab.com/reflow/reflow-usb-pd-headless) | Open the EasyEDA project and order both in a couple of clicks. Order the PCB first, components second so you can combine shipping.
2 | [22 AWG wire](https://www.aliexpress.com/item/1005003732230847.html) | For SMT stencil positioning.
3 | M1.6 countersunk SS screws 16 mm (×4) | [EDLW-S1-M1.6-L16](https://jlcmc.com/product/s/E02/EDLW/gb-t-819-phillips-countersunk-head-screw?k=EDLW-S1-M1.6-L16&productModelNumber=EDLW-S1-M1.6-L16). Mount the heater plate to the reflector. Make sure they are stainless steel.
4 | M1.6 SS spring lock washers (×8) | [EPDC-S1W-1.6](https://jlcmc.com/product/s/E06/EPDC/spring-washer-standard-spring-washers?k=EPDC-S1W-1.6&productModelNumber=EPDC-S1W-1.6). Hardware for the head mount.
5 | M1.6 SS nuts (×12) | [EMLA-S1W-BL1-M1.6](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M1.6&productModelNumber=EMLA-S1W-BL1-M1.6). Hardware for the head mount.
6 | M2 screws 3 mm (×8) | [EDDM-M2-L3](https://jlcmc.com/product/s/E02/EDDM/phillips-ultra-thin-head-screw?k=EDDM-M2-L3&productModelNumber=EDDM-M2-L3). Mount the reflector to the head base.
7 | M2 screws 6 mm (×4) | [EDDM-M2-L6](https://jlcmc.com/product/s/E02/EDDM/phillips-ultra-thin-head-screw?k=EDDM-M2-L6&productModelNumber=EDDM-M2-L6). Mount the PCB.
8 | M2 screws 10mm (×5) | [EDLU-S1-M-M2-L10](https://jlcmc.com/product/s/E02/EDLU/gb-t-818-823-phillips-pan-head-screw?k=EDLU-S1-M-M2-L10&productModelNumber=EDLU-S1-M-M2-L10). Mount the fan.
9 | M2 nuts (×5) | [EMLA-S1W-BL1-M2](https://jlcmc.com/product/s/E04/EMLA/hex-nut-standard-thin-thickened-coarse-fine-thread?k=EMLA-S1W-BL1-M2&productModelNumber=EMLA-S1W-BL1-M2). Mount the fan.
10 | [M2 insert nuts L=4mm OD=3.5 mm (×9)](https://www.aliexpress.com/item/1005007660074806.html) | Inserts for PCB mounts.
11 | [Magnets 6x6 mm (×8)](https://www.aliexpress.com/item/1005010074762903.html) | Cap lock. If you buy elsewhere, check the height and adjust the case. Two stacked 6x3 mm magnets also work.
12 | Magnets/inserts glue | Use an MMA-based adhesive such as [milky](https://www.aliexpress.com/item/1005002527252980.html) or [transparent](https://www.aliexpress.com/item/1005005299765624.html). A dual cartridge with a static mixer and a manual dispenser is strongly recommended. Using proper glue is important for a PA-12 nylon case.
13 | [0.4 mm bare copper wire](https://www.aliexpress.com/item/1005009540818990.html) | For soldering heater power. You can use solid copper wire 0.3–0.4 mm in diameter, or stranded wire of similar cross-section. The small cross-section helps reduce heat transfer.


### Optional

Treat this as a checklist; pick what you are missing from your bench.

&nbsp; | Name | Comment
-------|------|--------
1 | 140 W USB PD charger | A 28 V profile is required for the MCH heater (a 100 W charger is fine for the PCB heater). Support for a 21 V / 5 A PPS profile is highly recommended. [This one](https://www.aliexpress.com/item/1005008845375668.html) looks good and inexpensive.
2 | [K-probe thermometer](https://www.aliexpress.com/item/1005008715777280.html) | For temperature calibration. The TM-902C from AliExpress is inexpensive and accurate enough.
3 | [Soldering air gun](https://www.aliexpress.com/item/1005006099512955.html) | For PCB assembly.
4 | Low-temp soldering paste | Sn42Bi58, 138°C, for PCB assembly. Standard leaded paste works too, but low-temp paste is more convenient.
5 | 217°C soldering paste | Use it for the PCB heater power lines.
6 | [Heat-set insert tip](https://www.aliexpress.com/item/1005009572892377.html) | For a nylon (PA-12) case only. Select the one that fits your iron.
7 | [Zero-flute countersink](https://www.aliexpress.com/item/1005004217081267.html) | Select the 1 mm size. Countersink the PCB-based hotplate so the screw heads sit flush.


## JLCPCB / LCSC order notes

https://oshwlab.com/reflow/reflow-usb-pd-headless

1. `pcb_main`: 1.6 mm, green (other colors for 4-layer PCBs are pricey).
   - Stencil is strongly recommended, it adds roughly $3 to the order and saves
     a ton of effort:
     - Bottom side only.
     - Comment "**Make stencil according to paste mask file and don't forget the corner holes marked with arrows**". Otherwise, the positional holes might be missed.
     - Select "Custom size" and set 90 x 80 mm so the stencil stays compact and
       lightweight, keeping delivery costs down.
     - Select "Confirm production file" (ensure positional holes are properly generated)
2. `pcb_cap`: 1.6 mm, white.
3. `head_base`: 1.2 mm, white.
4. `head_reflector`: 1.6 mm, white.
5. `head_heater`: aluminum, 1.6 mm, 1 oz.
6. `foil_conductor`: 1.0 mm, red.

jlc3dp prints:

1. `tray`, `cap`: SLA "JLC black resin" or MJF "PA12-HP nylon".
   - SLA: okay, budget price.
   - Nylon: 2x more expensive than SLA, better-looking.
   - (!) With nylon, use `tray_insert_3.25` (reduced holes) for heat-set
     insertion into plastic. `tray` holes are 3.6 mm for glue.
2. `air_duct_pi5_top`, `air_duct_pi5_bottom`: MJF "PA12-HP nylon", unpainted (natural color).
3. `button`: SLA "8001 resin", translucent.
4. `tool_spacer`: SLA "9600 resin".
5. `pcb_aligner`: SLA "9600 resin".
6. `drill_conductor`: SLA "9600 resin".
7. `magnet_clamp`: SLA "9600 resin".
