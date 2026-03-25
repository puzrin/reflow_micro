include <../../src/lib/utils.scad>;

$fn = 64;

margin = 0.1;

mch_x = 69.9;
mch_y = 14.4;
mch_z = 1.29;

mch_pwr_x = 6;
mch_pwr_y = 8;

niche_x = mch_x + margin*2;
niche_y = mch_y + margin*2;
niche_z = mch_z - 0.1;

stencil_x = 100;
stencil_y = 46;

guide_sze = 2;

plate_x = stencil_x + margin*2 + guide_sze*2;
plate_y = stencil_y + margin*2 + guide_sze*2;
plate_z = 3;

difference() {
    // plate
    mirror_z() ra_cube([plate_x, plate_y, plate_z], r=2);

    // MCH inner
    tr_z(e)
    mirror_z() ra_cube([niche_x, niche_y, niche_z], r=0);

    // Corner champfers
    dupe_xy()
    translate([niche_x/2, niche_y/2, e])
    mirror_z() cylinder(d=2, h=niche_z);

    // Power cords reserve
    translate([niche_x/2 - mch_pwr_x, -mch_pwr_y/2, e])
    mirror_z() cube([100, mch_pwr_y, plate_z+1]);

    // Plastic saving
    dupe_xy()
    translate([7, plate_y/2-9, e])
    mirror_z() ra_cube([30, 30, plate_z+1], r=5, center=false);

    dupe_x()
    translate([-plate_x/2 + 9, -20/2, e])
    mirror_x() mirror_z() ra_cube([30, 20, plate_z+1], r=5, center=false);

};

// guides
dupe_xy() {
    translate([stencil_x/2 + margin, stencil_y/2 - 6, -e])
    ra_cube([guide_sze, guide_sze, guide_sze+0.5], r=0.6, rtop=0.9, center=false);

    translate([stencil_x/2 - 6, stencil_y/2 + margin, -e])
    ra_cube([guide_sze, guide_sze, guide_sze+0.5], r=0.6, rtop=0.9, center=false);
}
