include <lib/utils.scad>;
include <lib/nut_trap.scad>;

$fn = $preview ? 64 : 64;

draw_top = 0;
draw_bottom = 0;

wall = 1.5;
wall_base = 2;
bottom_dia = 16;
x = 10;
y = 18;
fan_h = 6.2;
fan_w = 28;
r = 5;

top_dia = 10;
top_h = 11;

mnt_wall = 2;
mnt_hole = 2.2;
mnt_y = 13.5;

module half_sphere (r) {
    difference() {
        sphere(r);
        d = r*2;
        tr_z(-(d+1)/2) cube([d+1, d+1, d+1], center=true);
    }
}

module mount_plate () {
    ra_cube([7, 32, wall_base], r=2);
}

module mount_plate_substract () {
    dupe_y() tr_y(mnt_y) {
        tr_z(-2*e) cylinder(mnt_wall+4*e, d = mnt_hole);

        rotate([0, 0, 38])
        tr_z(mnt_wall+e)
        nut_trap(h=fan_h+mnt_wall, clearance=0);
    }
}

module bottom () {
    difference () {
        union () {
            // body
            hull () {
                difference() {
                    dupe_y() tr_xy(-x/2+r, y/2-r) half_sphere(r + wall);
                    translate([x/2-e, -50, -e]) cube(100);
                }

                dupe_y() tr_xy(x/2, fan_w/2 + wall) cube([e,e, fan_h + wall]);
                dupe_y() tr_xy(x/2-1, fan_w/2 + wall) cube([e,e, fan_h + wall]);
            }

            mount_plate();
        }

        // inner
        translate([e, 0, -e]) hull () {
            dupe_y()
            tr_xy(-x/2+r, y/2-r)
            half_sphere(r);

            dupe_y() tr_xy(x/2, fan_w/2) cube([e,e, fan_h]);
        }

        mount_plate_substract();
    }
}

module top () {
    difference () {
        union () {
            // body
            hull () {
                dupe_y() tr_xy(-x/2+r, y/2-r) cylinder(e, r + wall);

                tr_z(top_h) mirror_z() cylinder(e, r = top_dia/2 + wall);
            }

            mount_plate();

            // orientation key
            tr_x(-x/2 - wall)
            linear_extrude(top_h) hull() {
                square([wall, wall], center = true);
                tr_x(-1.5 + wall/2) circle(d = wall);
            }
        }

        // inner
        tr_z(-e) hull () {
            dupe_y() tr_xy(-x/2+r, y/2-r) cylinder(e, r);
            tr_z(top_h) cylinder(h = 2*e, r = top_dia/2);
        }

        mount_plate_substract();
    }
}

if (!draw_top && !draw_bottom) {
    rotate_x(180) bottom();
    tr_z(2) top();
}

if (draw_top) { top(); }
if (draw_bottom) { bottom(); }
