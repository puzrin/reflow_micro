MCH-based heating head <!-- omit in toc -->
======================

- [BOM](#bom)
- [MCH heater resistance matching](#mch-heater-resistance-matching)
- [Cut SS clamps](#cut-ss-clamps)
- [Mount hotplate top parts](#mount-hotplate-top-parts)
- [Mount reflector](#mount-reflector)
- [Mount base](#mount-base)


## BOM

This BOM extends the default one; be sure to order it as well.

&nbsp; | Name | Comment
-------|------|--------
1 | Hotplate and clamps | See details below.
2 | M1.6 countersunk SS screws 8 mm (×6) | [EDLW-S1-M1.6-L8](https://jlcmc.com/product/s/E02/EDLW/gb-t-819-phillips-countersunk-head-screw?k=EDLW-S1-M1.6-L8&productModelNumber=EDLW-S1-M1.6-L8). MCH mount.
3 | [MCH 70*14 2R / 7R (×2)](https://www.aliexpress.com/item/32966428374.html) | Two heaters are required; buy 5-10 so you can match their resistance. Note: the 10R variant is actually 7R, but that's not guaranteed. Also select the proper base-board variant for the MCH resistance. 7R is recommended for better heating performance.
4 | [Thermally conductive paste](https://www.aliexpress.com/item/1005009116482403.html) | Note, paste should be durable at 250°C. Pastes from 3d printer hotends are good candidates.

**Hotplate (JLCCNC)**:

- File `hotplate_80x70x2.8.step`
  - Material: Aluminum 6061.
  - Surface Finish: "Anodizing" (Natural color, glossy).
  - Comment: "Face mill both top and bottom flat surfaces. No raw stock surface acceptable."

**MCH mounting (JLC3DP)**

- File `ss_mch_mount.stl`
  - `SLM(Metal)` process, 316L steel. Don't try `BJ(Metal)`; it will be rejected.
  - Comment: "All defects are acceptable in advance." The design is on the edge
    of the printing requirements, so this comment helps avoid order declines.

**Thermal paste stencil (JLCPCB)**

This is optional. It makes applying the thermally conductive paste easier.

https://oshwlab.com/puzrin/reflow-table-pd

- `thermal_paste_stencil`
  - Top side only.
  - Custom size 100x46 mm.
  - Thickness 0.1 mm.
  - Select "Confirm production file".
  - Comment "**Make stencil according to paste mask file**".

**Thermal paste conductor (JLC3DP)**

This stencil conductor is also optional.

- File `thermal_paste_conductor.stl`
  - SLA, LEDO 6060.

**Optional**

Treat this as a checklist; pick what you are missing from your bench.

&nbsp; | Name | Comment
-------|------|--------
1 | [Dental diamond disks](https://www.aliexpress.com/item/4001138228461.html) | For cutting the stainless clamps. Grab a mandrel too.
2 | Phosphoric Acid | To tin MCH wires if you don't have another active flux. [Dental Etchant Gel](https://www.aliexpress.com/i/1005006254467176.html) works pretty well. Soldering tip cleaner should be okay too, but it is less convenient than gel.


## MCH heater resistance matching

The build uses two MCH heaters, so their resistance needs to match closely for
balanced heating. Buy a few extras so you can pair them.

> [!NOTE]
>
> Aim for roughly 2% tolerance.

After the main board is assembled and flashed, use a temporary head base board
with a wire connector:

- Connect the device and open `Measure resistance` in `Settings`.
- Swap MCH heaters and record the resistance of each one.
- Sort by resistance and select the best-matched pairs.

> [!TIP]
>
> - Make sure the wire lengths are similar for all MCHs when measuring.
> - Buying ten heaters typically yields 3-4 usable pairs. Five heaters
>   typically yield 1-2 pairs.

TBD: image


## Cut SS clamps

Cut the stainless steel clamps free from the panel. A Dremel with thin dental
diamond disks works very well and keeps the cuts precise.

<img src="./images/ss_clamps.jpg" width="30%"> <img src="./images/ss_clamps_cutted.jpg" width="30%">


## Mount hotplate top parts

- Apply a very thin layer of thermal paste to the heaters.
- Place the heaters on the plate and slide them about +/-0.5 mm to spread the
  paste.
- Tighten the screws with spring washers using only a light touch.

Use 8 mm screws for the heaters and 18 mm screws for the corners.

<img src="./images/head_hotplate_mch_bottom.jpg" width="30%">

> [!CAUTION]
>
> Do not overtighten the screws; the MCH can crack after heating!

Once everything is mounted, cut the MCH wires to the required length and tin
the ends.

- Usually, a hot iron tip with plenty of solder and ordinary flux is enough.
  Try tinning the cut wires first to make sure it works.
- If you have problems, use active flux. Alternatives include "dental etchant
  gel" or "iron tip refresher".
- Remember to clean the wires after using active flux.


## Mount reflector

Use the conductor as a template to cut the foil reflector, then punch the holes
using a screw.

- Place the foil on a flat surface with the template on top.
- Cut the foil along the template outline and the central hole with a knife.
- Secure the foil to the template with tape to prevent shifting.
- Punch the mounting holes by pressing through with a screw.
- Clean up the foil burrs around the holes by scraping with a tilted screw.

<img src="../images/head_foil.jpg" width="30%">

Install the foil and frame on the hotplate.

- Use spacers to set a 12 mm gap between the heater top and the reflector.
- For each corner, insert the spacer, snug the top nut first, then tighten
  the bottom nut; spring lock washers go underneath.

<img src="./images/head_reflector.jpg" width="30%"> <img src="./images/head_reflector_spacer.jpg" width="30%">


## Mount base

Assemble and mount the base board the same way as described in the
[main build guide](../build.md).

<img src="./images/head_bottom.jpg" width="30%">
