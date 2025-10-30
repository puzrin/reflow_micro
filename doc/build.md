Device build <!-- omit in toc -->
============

- [PCB assembly](#pcb-assembly)
- [Firmware upload](#firmware-upload)
- [Heater assembly](#heater-assembly)
- [Case](#case)
- [Calibration](#calibration)
  - [Temperature sensor](#temperature-sensor)
  - [ADRC controller](#adrc-controller)


## PCB assembly

Pin the SMT stencil and PCB to a silicone pad with a loop of 22 AWG wire. Any
soft surface works - beer cork pads are great too.

Spread solder paste. Load enough paste so you can sweep it through the stencil
in a single pass with a plastic card. Multiple passes tend to overload the
openings and can short nearby pins.

Place the SMD parts and solder everything with a hot air gun. Inspect the board
and fix any issues with a soldering iron and flux. If you're going to wash the
PCB in an ultrasonic bath, leave the buzzer off until after cleaning so it
stays safe.

<img src="./images/pcb_bottom_no_fan.jpg" width="30%">

**Tip:** Use low-temperature (138°C) paste to keep the process easy and minimize
mistakes.

Then solder the heating head connectors on the top side of the PCB.

When you're done, install the fan and air duct on the PCB cover with the 10 mm
screws.

<img src="./images/pcb_with_fan.jpg" width="30%"> <img src="./images/pcb_with_fan_top.jpg" width="30%">


## Firmware upload

TBD


## Heater assembly

[See in separate file](./heater_assembly.md).


## Case

TBD


## Calibration

For reliable operation, calibrate the temperature sensor and the ADRC
controller from the `Settings` page.

### Temperature sensor

For heating heads with an RTD sensor you only need to enter the actual room
temperature.

For heads without sensors you need two points: room temperature and a
high-temperature reading. Open the app, connect the device, head to Settings,
and start the calibration.

- **Step 1.** Make sure the device is cold and enter the room temperature.
- **Step 2.** (only for heads without an RTD) Press the button to start heating
  with the suggested power (50-70 W) and wait until the temperature stabilizes.
  Place a small drop of solder on the table, touch it with the temperature
  probe, and enter the real temperature from the thermometer.

*Tip: Measuring through a solder drop gives the probe a better contact area.
Touching the plate directly with the small K-type bead leads to noticeable
errors.*

### ADRC controller

The default settings work well for 80x70x3 mm plates. If you're using another
size, recalibrate the ADRC controller.

TBD
