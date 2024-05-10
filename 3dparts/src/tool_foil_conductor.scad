include <utils.scad>;

$fn = 64;

plate_x = 80;
plate_y = 70;

module mch_corner_hole() {
    // Coordinates from easyeda
    translate([65-45, 64-40, 0]) cylinder(h=20, d=1.8, center=true);
}

module mch_middle_hole() {
    // Coordinates from easyeda
    translate([65-45, 0, 0]) cylinder(h=20, d=1.8, center=true);
}

module mch_holder_hole() {
    translate([plate_x/2 - 3, plate_y/2 -3, 0]) cylinder(h=20, d=1.8, center=true);
}

difference() {
    union() {
        rcube([plate_x-2, plate_y, 2], r=3);
        rcube([plate_x-2, 52, 3], r=0);
    }
    
    mch_corner_hole();
    mirror_x() mch_corner_hole();
    mirror_y() mch_corner_hole();
    mirror_x() mirror_y() mch_corner_hole();
 
    mch_middle_hole();
    mirror_x() mch_middle_hole();

    mch_holder_hole();
    mirror_x() mch_holder_hole();
    mirror_y() mch_holder_hole();
    mirror_x() mirror_y() mch_holder_hole();
}