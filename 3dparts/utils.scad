e = 0.01;
2e = 0.02;

module torus(r1, r2, $fn = $fn) {
    rotate_extrude(convexity=2, $fn=$fn)
    difference() {
        translate([r1 - r2, 0, 0]) circle(r=r2, $fn=$fn);
        translate([-r2, 0, 0]) square(r2*2, true);
    }
}

module _hooled_toruses(x, y, r_xy, r_z) {
    translate([r_xy, r_xy, 0]) torus(r_xy, r_z);
    translate([x-r_xy, r_xy, 0]) torus(r_xy, r_z);
    translate([r_xy, y-r_xy, 0]) torus(r_xy, r_z);
    translate([x-r_xy, y-r_xy, 0]) torus(r_xy, r_z);
    
    translate([r_z, r_xy, 0]) rotate([-90, 0, 0]) cylinder(y-r_xy*2, r=r_z);
    translate([x-r_z, r_xy, 0]) rotate([-90, 0, 0]) cylinder(y-r_xy*2, r=r_z);
    translate([r_xy, r_z, 0]) rotate([0, 90, 0]) cylinder(x-r_xy*2, r=r_z);
    translate([r_xy, y-r_z, 0]) rotate([0, 90, 0]) cylinder(x-r_xy*2, r=r_z);
    
    translate([r_z, r_z, -r_z])
    rcube([x-r_z*2, y-r_z*2, r_z*2], r = r_xy-r_z, center = false);
}

module rsquare(size = [30, 10], r = [1, 1, 1, 1], center = true) {
    _r0 = is_num(r) ? r : r[0];
    _r1 = is_num(r) ? r : r[1];
    _r2 = is_num(r) ? r : r[2];
    _r3 = is_num(r) ? r : r[3];
    r0 = _r0 ? _r0 : e;
    r1 = _r1 ? _r1 : e;
    r2 = _r2 ? _r2 : e;
    r3 = _r3 ? _r3 : e;

    x = size[0]; y = size [1];

    tx = center ? -x/2 : 0;
    ty = center ? -y/2 : 0;

    tr_xy(tx, ty)
    hull () {
        tr_xy(r0, r0) circle(r0);
        tr_xy(r1, y - r1) circle(r1);
        tr_xy(x - r2, y - r2) circle(r2);
        tr_xy(x - r3, r3) circle(r3);
    }
}

module rcube(size = [10, 10, 10], center = true, r = 1, rtop = 0, rbottom = 0) {
    x = size[0]; y = size[1]; h = size[2];

    tx = center ? -x/2 : 0;
    ty = center ? -y/2 : 0;

    translate([tx, ty, 0]) {
        if (rtop == 0 && rbottom == 0) {
            translate([r, r, 0]) cylinder(h, r = r);
            translate([x-r, r, 0]) cylinder(h, r = r);
            translate([r, y-r, 0]) cylinder(h, r = r);
            translate([x-r, y-r, 0]) cylinder(h, r = r);
            translate([0, r, 0]) cube([x, y-r*2, h]);
            translate([r, 0, 0]) cube([x-r*2, y, h]);
        }
        if (rtop > 0 && rbottom == 0) {
            translate([0, 0, h-rtop]) _hooled_toruses(x, y, r, rtop);
            rcube([x, y, h-rtop], r = r, center = false);
        }
        if (rtop == 0 && rbottom > 0) {
            translate([0, 0, rbottom]) _hooled_toruses(x, y, r, rbottom);
            translate([0, 0, rbottom])
            rcube([x, y, h-rbottom], r = r, center = false);
        }
        if (rtop > 0 && rbottom > 0) {
            translate([0, 0, h-rtop]) _hooled_toruses(x, y, r, rtop);
            translate([0, 0, rbottom]) _hooled_toruses(x, y, r, rbottom);
            translate([0, 0, rbottom])
            rcube([x, y, h-rbottom-rtop], r = r, center = false);            
        }
    }
}

module rotate_x(angle) { rotate([angle, 0, 0]) children(); }
module rotate_y(angle) { rotate([0, angle, 0]) children(); }
module rotate_z(angle) { rotate([0, 0, angle]) children(); }

module tr_x(ofs) { translate([ofs, 0, 0]) children(); }
module tr_y(ofs) { translate([0, ofs, 0]) children(); }
module tr_z(ofs) { translate([0, 0, ofs]) children(); }
module tr_xy(ofs_x, ofs_y) { translate([ofs_x, ofs_y, 0]) children(); }

module mirror_x() { mirror([1, 0, 0]) children(); }
module mirror_y() { mirror([0, 1, 0]) children(); }
module mirror_z() { mirror([0, 0, 1]) children(); }
