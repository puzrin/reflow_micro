Heating Heads
=============

The reflow table supports two heater types you can build at home without
industrial tools or a dedicated workshop.

Keep in mind this table and the bottom-heating approach are a compromise between
compact comfort and rich features. It shines in a home hobby setup, but for
professional use you may still want a larger, more advanced reflow oven.

When a reflow profile runs, there's no cover cap and all heat comes from the
bottom. The plate and the top of the PCB can differ by as much as 20°C. Using
low-temperature (138°C) paste gives you plenty of margin, forgives minor
mistakes, and keeps the heater requirements modest.


## Aluminum PCB based heater

**Pros:**

- Very affordable.
- Easy to build.
- No temperature sensor required (use the TCR-based method).
- A 100 W charger is fine for an 80x70 mm plate.

**Cons:**

- Max temperature is only 180°C.
- Runs above the PCB's MOT (130°C) and will slowly degrade, but it's fine for
  hobby use that isn't 24/7.

Conclusion:

- Great for hobby builds that use LTS soldering.
- Handy as a bottom pre-heater for hot air work with leaded or RoHS pastes.
- Overall the recommended starting point.


## MCH-based heater (experimental)

**Pros:**

- Reaches up to 300°C.

**Cons:**

- Noticeably more expensive and involved.
- Needs a 140 W supply.
- Right now the dynamics above 200°C are weak - roughly 1°C per second (work in
  progress).
- Overall there's little headroom for leaded or RoHS reflow profiles. Each
  profile has to be tuned to the specific PCB, usually with a foil cap. It's too
  easy to damage a board, so beginners should steer clear.

Conclusion:

- Use it only if you understand the risks or want to experiment with improving
  the MCH heater.
- Works for long-term heating at high temperatures (paint baking, etc.).

This heater is still a proof of concept for advanced users. We're waiting on PD
3.2 chargers, which should start to appear around 2026. Once AVS profiles are
common, pushing more power into these heaters - and getting better dynamics -
will be easier.
