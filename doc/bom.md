Bill of Materials
=================

Note: If any AliExpress links break, use the description to track items down in
other shops. The important picks are also commented.

### General

&nbsp; | Name | Comment
-------|------|--------
1 | [PCB & Components](https://oshwlab.com/reflow/reflow-usb-pd-headless) | Open the EasyEDA project and order both in a couple of clicks. Order the PCB first, components second so you can combine shipping.
2 | [22 AWG wire](https://www.aliexpress.com/item/1005003732230847.html) | SMT stencil positioning.
3 | [M1.6 SS screws 18mm, cone cap](https://www.aliexpress.com/item/32946954901.html) | DIN965/GB819. Mount the heater plate to the reflector. Make sure they are stainless steel.
4 | [M1.6 screws 3mm, flat cap](https://www.aliexpress.com/item/4000308042674.html) | Mount the reflector to the head base.
5 | [M1.6 screws 10mm, flat cap](https://www.aliexpress.com/item/4000308042674.html) | Mount the fan.
6 | [M1.6 screws 6mm, flat cap](https://www.aliexpress.com/item/4000308042674.html) | Mount the PCB.
7 | [M1.6 SS spring lock washers](https://www.aliexpress.com/item/32975233438.html) | Hardware for the head mount.
8 | [M1.6 SS nuts](https://www.aliexpress.com/item/1005007593861199.html) | Hardware for the head mount.
9 | [M1.6 insert nuts 4mm](https://www.aliexpress.com/item/1005008644449489.html) | Inserts for the MCH mounts.
10 | [Magnets 6x6mm](https://www.aliexpress.com/item/1005005114069840.html) | Cap lock. If you buy elsewhere, check the height and adjust the case. Two stacked 6x3 mm magnets also work.
11 | [esp32-c3](https://www.aliexpress.com/item/1005004386637738.html) (optional) | If it isn't on LCSC, grab the ESP32-C3-WROOM-02-N4 instead.
12 | PA glue | For the PA-12 nylon case. Use an MMA-based adhesive such as [milky](https://www.aliexpress.com/item/1005002527252980.html) or [transparent](https://www.aliexpress.com/item/1005005299765624.html). A dual cartridge with a static mixer and a manual dispenser is strongly recommended.
13 | Zero-flute countersink | Countersink the PCB-based hotplate so the screw heads sit flush.
14 | [0.5mm bare copper wire](https://www.aliexpress.com/item/1005009540818990.html) | Solder heater power
15 | [M2 insert nuts 4mm OD 3.2/3.5](https://www.aliexpress.com/item/1005007660074806.html) | Inserts PCB mounts. 3.2mm for nylon (thermal insert), 3.5mm other.
16 | [M2 screws 3mm, flat cap](https://www.aliexpress.com/item/32981974171.html) | Mount the reflector to the head base.
17 | [M2 screws 6mm, flat cap](https://www.aliexpress.com/item/32981974171.html) | Mount the PCB.

Note: Strongly consider ordering the SMT stencil with your PCB. It adds roughly
$8 to the order and saves a ton of effort.


### Other (optional)

Treat this as a checklist; pick what you are missing from your bench.

&nbsp; | Name | Comment
-------|------|--------
1 | 140 W USB PD charger | A 28 V profile is required for the MCH heater (a 100 W charger is fine for the PCB heater). Support for a 21 V / 5 A PPS profile is highly recommended. [This one](https://www.aliexpress.com/item/1005008845375668.html) looks good and cheap.
2 | [K-probe thermometer](https://www.aliexpress.com/item/1005008715777280.html) | For temperature calibration. The TM-902C from AliExpress is inexpensive and accurate enough.
3 | [Soldering air gun](https://www.aliexpress.com/item/1005006099512955.html) | For PCB assembly.
4 | Low-temp soldering paste | Sn42Bi58, 138°C, for PCB assembly. Standard leaded paste works too, but low-temp paste is more convenient.
7 | 217°C soldering paste | Use it for the RTD wires and the PCB heater power lines.


## JLCPCB / LCSC order notes

https://oshwlab.com/reflow/reflow-usb-pd-headless

1. `pcb_main`: 1.6 mm, green (other colors for 4-layer PCB are pricey).
   - Option "Remove mark".
   - Stencil is strongly recommended.
     - Bottom side only.
     - Comment "**Make stencil according to paste mask file and don't forget the corner holes marked with arrows**". Otherwise the positional holes might be missed.
     - Select "Custom size" and set 90x80 so the stencil stays compact and
       light, keeping delivery costs down.
2. `pcb_cap`: 1.6 mm, white.
   - Comment: "PCB has no traces, mask only."
3. `head_base`: 1.2 mm, white.
4. `head_reflector`: 1.6 mm, white.
   - Comment: "PCB has no traces, mask only."
5. `head_heater`: aluminum, 1.6 mm, 1 oz.
6. foil_conductor: 1.0 mm, red.
   - Comment: "PCB has no traces, mask only."

jlc3dp prints:

1. `tray`, `cap`: SLA "JLC black resin" or MJF "PA12-HP nylon".
   - SLA: ok, budget price.
   - Nylon: 2x more expensive than SLA, better look.
   - (!) With nylon use `tray_insert_3.25` (reduced holes) for heat-set
     insertion into plastic. `tray` holes are 3.6 mm for glue.
2. `air_duct_pi5_top`, `air_duct_pi5_bottom`: MJF "PA12-HP nylon", non-painted (natural color).
3. `button`: SLA "8001 resin", translucent.
4. `tool_spacer`: SLA "9600 resin".
5. `pcb_aligner`: SLA "9600 resin".
6. `drill_conductor`: SLA "9600 resin".
