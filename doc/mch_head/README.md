MCH-based heating head <!-- omit in toc -->
======================

- [BOM](#bom)
- [MCH heaters resistance matching](#mch-heaters-resistance-matching)
- [Cut SS clamps](#cut-ss-clamps)
- [Mount hotplate top parts](#mount-hotplate-top-parts)
- [Mount reflector](#mount-reflector)
- [Mount base](#mount-base)


## BOM

&nbsp; | Name | Comment
-------|------|--------
1 | Hotplate and clamps | Details below in the JLCPCB notes.
2 | [M1.6 SS screws 18mm, cone cap](https://www.aliexpress.com/item/32946954901.html) | DIN965/GB819. Mount the heater plate to the reflector.
3 | [M1.6 SS screws 8mm, cone cap](https://www.aliexpress.com/item/32946954901.html) | DIN965/GB819. MCH Mount.
3 | [MCH 70*14 2R](https://www.aliexpress.com/item/32966428374.html) | Two heaters are required; buy 5-10 to match their resistance.
4 | [PT100 RTD sensor](https://www.aliexpress.com/item/1005007238778907.html) | Temperature feedback.
5 | [Soft silicone wire, 30 AWG, white](https://www.aliexpress.com/item/4001283806251.html) | RTD wiring.
6 | [Thermal conductive paste](https://www.aliexpress.com/item/1005002400161049.html) | Use Thermal Grizzly Kryonaut. The paste must handle 300°C, so skip the cheap stuff.
7 | [Milliohm meter](https://www.aliexpress.com/item/1005006408703765.html) | For pairing the MCH heaters.
8 | [Dental diamond disk](https://www.aliexpress.com/item/4001138228461.html) | For cutting the stainless clamps. Grab a mandrel too.
9 | Black paint | High-temperature aerosol paint (grill/stove/engine type) to coat the plate top.

Hotplate (jlc3dp, CNC):

- File `hotplate_80x70x2.8.step`
- Material: Aluminum 6061.
- Surface Finish: "Bead blasting".
- Comment: "Don't deburr".

MCH/Sensor mounting (jlc3dp, SS print)

- File `ss_mch_mount.stl`
- `SLM(Metal)` process, 316L steel. Don't try `BJ(Metal)`; it will be rejected.


## MCH heaters resistance matching

The table uses two MCH heaters, so their resistance needs to match closely for
balanced heating. Buy a few extras so you can pair them.

Aim for roughly 2% tolerance. Use a milliohm meter (see the optional purchases
list) and connect Kelvin clips directly to the MCH pads for accurate numbers.

Buying ten heaters typically yields three or four usable pairs.


## Cut SS clamps

If you printed stainless steel clamps, cut them free from the panel. A Dremel
with thin dental diamond disks works very well and keeps the cuts precise.

<img src="./images/ss_clamps.jpg" width="30%"> <img src="./images/ss_clamps_cutted.jpg" width="30%">


## Mount hotplate top parts

- Apply a very thin layer of thermal paste to the heaters.
- Place the heaters on the plate and slide them about +/-0.5 mm to spread the
  paste.
- Tighten the screws with spring washers using only a light touch.

Use 8 mm black screws for the heaters and RTD, and 18 mm screws for the corners.

<img src="./images/head_top_back.jpg" width="30%">

*Note: Do not overtighten the screws; the MCH can crack after heating!*

Once everything is mounted, solder 30 mm silicone wires to the RTD.

- Use high-temperature lead-free (217°C) paste.
- Keep the solder joints as small as possible.
- Strip and tin 1-2 mm on one end of each wire and 4-5 mm on the other. Add a
  little paste to the short ends and solder them to the RTD.
- Clean the flux with IPA to reduce heat absorption.


## Mount reflector

Use the conductor as a template to cut the foil reflector and punch the holes
with a screw.

<img src="./images/reflector_foil.jpg" width="30%">

Use spacers to set a 12 mm gap between the heater top and the reflector. For
each corner, insert the spacer, snug the top nut first, then tighten the bottom
nut (spring lock washers go underneath).

<img src="./images/head_reflector.jpg" width="30%"> <img src="./images/head_reflector_spacer.jpg" width="30%">


## Mount base

Start by soldering the connectors. They need precise alignment, so use the
conductors as jigs.

- Place the conductors on the main board corners and rest the female connectors
  on them.
- Don't push the connectors in; just align them over the male header holes.
- Position the heater base board on top and tack a few pins to lock it in
  place.
- Remove the heater base and finish soldering all the pins.

<img src="./images/pcb_with_conductors.jpg" width="30%"> <img src="./images/pcb_with_head_base.jpg" width="30%">

Now it's time for final head assembly.

- Install the 4 mm insert nuts with the 3 mm screws in the heating head base.
- Dry-fit the base, mark the MCH wires, and cut them to length.
- Remove the base and tin the MCH wires. Acid flux or an iron tip refresher
  works fine.
- Screw the head base to the reflector with the 3 mm screws.
- Solder the MCH and RTD wires.

<img src="./images/head_bottom.jpg" width="30%">

**Note.** You may need an active flux to tin the MCH wires. Try an "iron tip
refresher", use a small amount, and clean it off afterward.

Push the pin terminals into the main PCB for alignment, place the heater on
them, and solder.

TBD image.

Wash out the flux after soldering.
