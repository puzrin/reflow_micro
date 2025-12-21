Calibration
===========

Before you put the head to work, tune two things:

- Temperature sensor.
- Temperature controller.

Both are available from the `Settings` page in the menu.

Every head keeps its own settings in the on-board EEPROM, so you only need to
calibrate it once.


## Temperature sensor

Enter two real-world values: room temperature and the approximate maximum
operating temperature. A basic K-probe thermometer from the BOM is totally
fine.

These budget thermometers can drift by a couple of degrees. That's usually not a
deal-breaker, but if you prefer more accuracy, compare it with a known-good room
thermometer and note the delta. The Xiaomi Mijia BT thermometer, for example,
claims only 0.5°C error.

Open the app and go to the temperature calibration page.

### Room temperature entry

- Let the thermometer and device sit for 15 minutes with no drafts. Turn off
  the air conditioner if it's running.
- Enter the thermometer reading (Point 1).

After this step the web app will be close to the real temperature, but you still
need the second point.

### Max temperature entry

Now you need to bake the sensor a bit below the max operating value.

- 170°C for PCB-based heater. ~ 25 W.
- 250°C for MCH-based heater. ~ 50 W.

Don't exceed the maximum or you can damage the heater.

- Set the recommended power and press `Bake`.
- Wait for the temperature to stabilize. It can take 10-15 minutes.
- Monitor the temperature with the thermometer through a solder drop and tweak
  the power if needed.
- Enter the Point 2 value.

*Tip: Measuring through a solder drop gives the probe a better contact area.
Touching the plate directly with the small K-type bead leads to noticeable
errors.*


## Temperature controller

The default settings are for MCPCB-based heater, so there's a
good chance you won't need to touch the ADRC values.

For an MCH-based heater, or if you change the heater size, follow the
instructions below.

- Enable debug info on the `Settings` page.

### Run a step response test

This automatically estimates the head response time and scale factor.

- Wait until the hotplate cools back to room temperature. If it was warm, give
  it 10-15 minutes, or use the cooling fan.
- In the `Auto tuning` section, set the power as you did for the sensor bake.
  Press `Run` and wait for it to finish.

### Tune N/M parameters

- Set `M` = 2 and don't change it.
- In the `Test controller` section, set the temperature to the main working range,
  depending on heater type (170-210°C), and press `Run`.
- When the temperature stabilizes, start increasing `N` from 10 until you see
  power jitter > 20% in debug info.
- After jitter appears, reduce `N` by 10-20%.

Usually, this is OK:

- MCPCB head: `M = 2`, `N = 50`.
- MCH head: `M = 2`, `N = 60`.
