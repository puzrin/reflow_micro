Soldering paste
===============

As explained in [Heating Heads](./heating_heads.md), this reflow table is tuned
for comfortable home use with accessible tools. Here are a few facts about
low-temperature pastes:

1. Keep the operating temperature under 80-90°C. Avoid very hot MOSFETs and
   similar situations.
2. Pure SnBi pastes are brittle; SnBiAg blends hold up better.
3. Avoid high-vibration environments (automotive, etc.). That's rarely an issue
   at home, but for connectors or wires with heavy mechanical stress consider
   leaded or standard RoHS solder.
4. AliExpress pastes vary wildly in quality. Buy a known brand like Chip Quik
   whenever you can.

What that means in practice:

- If you gamble on SnBi paste from AliExpress, test it first. It's normal for
  70-80% of random listings to be duds.
- Grab a branded paste like TS391LT when possible, or pick one up from a
  trusted local supplier.


## Some tips

- Store paste in a syringe. Jars dry out within months after opening, while a
  syringe can keep the paste usable for years.
- If the paste separates because of rough shipping or long storage, don't
  worry. Connect two syringes with a luer-lock adapter and pump the paste back
  and forth a few times. It will mix back to normal.
- Avoid ordering paste in the peak of summer when long deliveries sit in high
  heat.


## Applying paste with SMT stencil

- Rule number one: don't be stingy. Load enough paste on the stencil and make a
  single pass with the squeegee. Most problems show up when you try to use too
  little paste and make several passes.
- A plastic card works well as a squeegee.


## Reflow with LTS paste

The default LTS reflow profile in the table has plenty of headroom for most
heavy boards:

- Temperature points are about +10°C higher than usual to cover losses from
  bottom heating.
- The top dwell stretches to 60 seconds.

That profile worked fine on a 4-layer 1.6 mm board with two internal copper
layers. Because LTS paste runs at relatively low temperatures, it's forgiving
and unlikely to overcook the board.

---

**NOTE:** This is practical, hobby-focused advice rather than a professional
process guide.
