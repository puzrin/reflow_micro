Heating Heads
=============

The reflow table supports two heater types you can build at home without
industrial tools or a dedicated workshop.

Keep in mind that this table uses a bottom-heating approach, which is a
compromise between compactness and feature richness. It shines in a home hobby
setup, but for professional use you may still want a larger, more advanced
reflow oven.

When a reflow profile runs, there's no cover cap and all heat comes from the
bottom. The plate and the top of the PCB can differ by as much as 20°C. Using
low-temperature (138°C) paste gives you plenty of margin, forgives minor
mistakes, and keeps the heater requirements modest.


## Aluminum PCB-based heater

**Pros:**

- Very affordable.
- Easy to build.
- A 100 W charger is fine for an 80x70 mm plate.

**Cons:**

- Max temperature is only 180°C.
- Runs above the MCPCB's MOT (130°C) and will slowly degrade, but it's fine for
  hobby use that isn't 24/7.

Conclusion:

- Great for hobby builds that use LTS soldering.
- Handy as a bottom pre-heater for hot air work with leaded or RoHS pastes.
- Overall, it's the recommended starting point.


## MCH-based heater

**Pros:**

- Safe up to 250°C.
- Suitable for SnPb paste.

**Cons:**

- A bit more expensive and involved, with more components to buy.
- Needs a 140 W supply.
- Performance above 200°C is not ideal: roughly 1°C per second.

Conclusion:

- If you need SnPb paste, use it, but do not expect good results with RoHS
  paste.

We're waiting on PD 3.2 chargers, which should start appearing around 2026.
Once AVS profiles are common, pushing more power into these heaters
(and getting better dynamics) will be easier.
