Device assembly <!-- omit in toc -->
===============

- [Required components](#required-components)
  - [General](#general)
  - [Other (optional)](#other-optional)
- [JLCPCB / LCSC order notes](#jlcpcb--lcsc-order-notes)
- [PCB assembly](#pcb-assembly)
- [Firmware upload](#firmware-upload)
- [Heater assembly](#heater-assembly)
- [Calibration](#calibration)
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
7 | [MCH 70*14 1R](https://www.aliexpress.com/item/32966428374.html) | 2 heaters required, but buy 5-10 to match resistance.
8 | Aluminum Foil 50μm (0.05mm) | Heater bottom insulation. Such foil is sold for sauna insulation. You can also try foil from baking forms.
9 | [Thermal conductive paste](https://www.aliexpress.com/item/1005006085448629.html) | [Alternate](https://www.aliexpress.com/item/32870824982.html). Should work at 300°C. Don't try cheap ones.
10 | [Magnets 6x6mm](https://www.aliexpress.com/item/1005005114069840.html) | Cap lock. If buy in other place - check height and adjust case appropriately.
11 | [esp32-c3](https://www.aliexpress.com/item/1005004386637738.html) (optional) | If not available at LCSC, model ESP32-C3-WROOM-02-N4.
12 | Black paint | High temperature paint, aerosol (used for barbecue / stovespray), to cover heating plate top.

Note. You are strongly advised to order SMT stencil for your PCB. That will
add ~ 8$ in total to your order - good price for convenience.


### Other (optional)

&nbsp; | Name | Comment
-------|------|--------
1 | 100W USB PD changer with PPS | 21v/5A PPS profile required. [Voltme 100W](https://www.aliexpress.com/item/1005004624922429.html), [Voltme 140W](https://www.aliexpress.com/item/1005004777502660.html), [Essager 100W](https://www.aliexpress.com/item/1005006436990810.html). If you prefer different one - make sure required power profile is supported, that's important.
2 | [Digital thermometer](https://www.aliexpress.com/item/32815540975.html) | [Alternate](https://www.aliexpress.com/item/32803473451.html) | For temperature calibration.
3 | [Soldering air gun](https://www.aliexpress.com/item/1005006099512955.html) | For PCB assembly.
4 | [Low temp soldering paste](https://www.aliexpress.com/item/1005006023229246.html) | Sn42bi58, 138°C, for PCB assembly. [Alternate](https://www.aliexpress.com/item/1005006724027713.html).
5 | [Milliohm meter](https://www.aliexpress.com/item/1005006408703765.html) | For MCH heaters pairing.


## JLCPCB / LCSC order notes

https://oshwlab.com/reflow/reflow-usb-pd-headless

1. PCB_main: 1.6mm, green.
   - Stencil strongly recommended.
     - Bottom side only.
     - Comment "**Make stencil according to paste mask file and don't
       forget corner holes, marked with arrows**". Or positional holes can be missed.
     - Select "custom size", and set 90\*80. Then stencil will be compact and
       light, with small delivery cost.
2. PCB_cap: 1.2mm, white.
   - Comment: "PCB has no traces, mask only, data is correct."
3. hotplate_base: 1.2mm, white.
   - Option "Remove order number".
4. hotplate_reflector: 1.6mm, white.
   - Comment: "PCB has no traces, mask only, data is correct."
5. MCH mount: aluminum, 1.0mm
   - Comment: "This PCB has no traces and no mask, data is correct."
6. foil_conductor: 0.8mm, green.
   - Comment: "PCB has no traces, mask only, data is correct."

jlc3dp, CNC:

1. Hotplate:
   - Material: Aluminum 6061.
   - Surface Finish: "As machined"..
   - Tightest Tolerance: 0.05.
   - Surface Roughness: Ra1.6.
   - Comment: "Please don't deburr".

jlc3dp, printed:

1. tray & cap: SLA "black resin" or MJF "PA12-HP nylon".
2. button: SLA "8001 resin", translucent.
3. tool_spacer: SLA "LEDE 6060"


## PCB assembly

Since PCB can be used for MCH matching and paint baking, worth build it first.
You are strongly advised use SMT stencil and low temperature soldering paste.
That will simplify things and minimize mistakes.

Pin SMT stencil and PCB with 22 AWG wire to silicon pad. You can also any soft
surface - beer cork pads, for example.

TBD

Apply soldering paste. Use enough paste, to apply with single move of plastic
card. If you move card over SMT stencil holes multiple times - paste can be
overdosed, and some pins can be shorted.

Place SMD components and solder all with air gun. Inspect result and fix defects
with soldering iron (and flux) if needed. If you plan to clean PCB from flux in
ultrasonic bath - postpone buzzer mount to avoid damage.

## Firmware upload

TBD

## Heater assembly

[See in separate file](./heater_assembly.md).


## Calibration

For good hotplate temperature interpolation, 2 points must be set. We use room
temp as first point and 200-300°C as second point.

Open application, connect your device, go to settings and press calibration
button.

- **Step 1**. Make sure device is "cold", and enter room temperature.
- **Step 2**. Press button to start heating with suggested power (50-70W), and
wait until temperature become stable (you will see approximate value). Then,
place a small peace of solder on the table, and put temperature sensor into it.
Enter the real value, showed by thermometer.

Now, device is calibrated and ready to use.

*Note. It's important to use solder drop for temperature measure instead of
direct touch. K-pair sensor has a small ball on the end - contact surface
without solder will be very small, that will cause notable errors.*

## Case

TBD
