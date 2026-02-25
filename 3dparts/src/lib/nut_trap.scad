// M2 by default
module nut_trap(h = 1.6, side_depth = 0, nut_s = 4.0, clearance = 0.15) {
    // h     - nut thickness
    // side_depth     - slot length from nut center to column edge
    // nut_s     - nut width across flats
    // clearance - tolerance

    nut_r = (nut_s + clearance * 2) / 2 / cos(30);
    slot_w = nut_s + clearance * 2;
    pocket_h = h + clearance * 2;

    union() {
        // Hexagonal pocket
        cylinder(h = pocket_h, r = nut_r, $fn = 6);

        // Entry slot
        if (side_depth > 0)
            translate([0, -slot_w / 2, 0])
                cube([side_depth, slot_w, pocket_h]);
    }
}