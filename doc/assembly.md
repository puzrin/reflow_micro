Device assembly <!-- omit in toc -->
===============

- [Required components](#required-components)
  - [General](#general)
- [JLCPCB / LCSC order notes](#jlcpcb--lcsc-order-notes)
- [Heater assembly](#heater-assembly)
- [PCB assembly](#pcb-assembly)
- [Firmware upload](#firmware-upload)
- [Case](#case)


## Required components

### General

&nbsp; | Name | Comment
-------|------|--------
1 | [PCB & Components](https://oshwlab.com/reflow/reflow-usb-pd-headless) | Go to EasyEda project page and order both in couple of clicks. If you order PCB first, components second - you will be able to join delivery. Add rocker switch below to LCSC order.
2 | [22 AWG wire](https://www.aliexpress.com/item/1005003732230847.html) | SMT stencil positioning.
3 | [DIN965 M1.6 SS screws 20mm](https://www.aliexpress.com/item/33013472653.html) | Mount heater plate to PCB. Should be stainless steel, important!
4 | [DIN965 M1.6 black screws 8mm](https://www.aliexpress.com/item/4000217127933.html) | MCH mount
5 | [M1.6 SS washers](https://www.aliexpress.com/item/4000222547150.html) | MCH mount
6 | [M1.6 SS spring lock washers](https://www.aliexpress.com/item/4000222556028.html) | MCH mount
7 | [MCH 50\*50\*2mm 150W](https://www.aliexpress.com/item/33005272347.html) | [Alternate](https://www.aliexpress.com/item/32991559341.html). Minimal power at room temp - 130W. For 110 volts buy 300W one.
8 | Aluminum Foil 50μm (0.05mm) | Heater bottom insulation. Such foil is sold for sauna insulation. You can also try foil from baking forms.
9 | [Thermal conductive paste](https://www.aliexpress.com/item/1005006085448629.html) | [Alternate](https://www.aliexpress.com/item/32870824982.html). Should work at 300C. Don't try cheap ones.
10 | [Magnets 6x6mm](https://www.aliexpress.com/item/1005005114069840.html) | Cap lock. If buy in other place - check height and adjust case appropriately.
11 | [esp32-c3](https://www.aliexpress.com/item/1005004386637738.html) (optional) | If not available at LCSC, model ESP32-C3-WROOM-02-N4.

Note. You are strongly advised to order SMT stencil for your PCB. That will
add ~ 8$ in total to your order - good price for convenience.


## JLCPCB / LCSC order notes

1. PCB:
   - Thikness 1.6mm, white mask advised for better look.
   - Stencil strongly recommended.
     - Select stencil for bottom side.
     - Add order comment "**make stencil according to paste mask file and don't
       forget corner holes, marked with arrows**". Or positional holes can be missed.
     - Select "custom size", and set 95\*85. Then stencil will be compact and
       light, with small delivery cost.
2. PCB shield:
   - aluminum, thickness 1.2mm.
   - Select option "Remove order number"
3. Reflector:
   - aluminum, thickness 1.2mm.
   - Select option "Remove order number"
4. MCH mount - aluminum, thickness 1.2mm
5. Hotplate (CNC machined):
   - Aluminum 6061
   - Hardcoat anodizing, black
6. Case:
   - Top cover and tray SLA CBY resin or SLS 1172pro Nylon for better look.
   - Button - SLA 8001 resin, translucent.


## Heater assembly

All details are  in [separate document](heater_assembly.md) to keep this one
compact.

That's the most boring part. Read is carefully before start making this device.


## PCB assembly

Pin SMT stencil and PCB with 22 AWG wire to silicon pad.

TBD

Apply soldering paste, place SMD components and solder all with air gun.
Inspect result and fix defects with soldering iron if needed. If you plan to
clean PCB from flux in ultrasonic bath - postpone buzzer mount to avoid damage.

TBD

Mount heater.

TBD

Now everything ready to be flashed and placed into case.


## Firmware upload

TBD


## Case

TBD
