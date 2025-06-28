include <utils.scad>;

$fn = 64;

draw_top = 0;
draw_bottom = 0;

wall = 1.5;
wall_base = 2;
bottom_dia = 16;
x = 10;
y = 18;
back_inner_h = 6.2;
r = 5;

top_dia = 10;
top_h = 10;

mnt_wall = 2;
mnt_w = 5;
mnt_hole = 1.8;
mnt_y = 13.5;

conic=1;

module bottom () {
    difference () {
        union () {
            // body
            hull () {
                dupe_y() tr_xy(-x/2+r, y/2-r) {
                    tr_z(back_inner_h-r) sphere(r + wall);
                    cylinder(e, r + wall);
                }
                dupe_y() tr_xy(x/2, 30/2-1.25 + wall) cube([e,e, back_inner_h + wall]);
            }

            // mounting
            rcube([6, 32, wall_base], r=3);
        }

        // inner
        translate([e, 0, -e]) hull () {
            dupe_y() tr_xy(-x/2+r, y/2-r) {
                tr_z(back_inner_h-r) sphere(r);
                cylinder(e, r);
            }
            dupe_y() tr_xy(x/2, 30/2-1.25) cube([e,e, back_inner_h]);
        }

        mirror_z() translate([-50, -50, -e]) cube(100);
        translate([x/2-e, -50, -e]) cube(100);

        // mount holes
        dupe_y() tr_y(mnt_y) {
            tr_z(-e) cylinder(mnt_wall+2*e, d = mnt_hole);
            tr_z(mnt_wall) cylinder(100, d = 4.0);
        }
    }
}

module top () {
    difference () {
        union () {
            // body
            hull () {
                dupe_y() tr_xy(-x/2+r, y/2-r) cylinder(e, r + wall);

                if (conic) {
                    tr_z(top_h) mirror_z() cylinder(e, r = top_dia/2 + wall);
                } else {
                    tr_z(top_h) dupe_y() tr_xy(-x/2+r, y/2-r) cylinder(e, r + wall);
                }
            }

            // mounting
            rcube([6, 32, wall_base], r=3);

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

            if (conic) {
                tr_z(top_h) cylinder(h = 2*e, r = top_dia/2);
            } else {
                tr_z(top_h+2*e) dupe_y() tr_xy(-x/2+r, y/2-r) cylinder(e, r);
            }
        }

        // mount holes
        dupe_y() tr_y(mnt_y) {
            tr_z(-e) cylinder(mnt_wall+2*e, d = mnt_hole);
            tr_z(mnt_wall) cylinder(100, d = 4.5);
        }
    }
}

if (!draw_top && !draw_bottom) {
    rotate_x(180) bottom();
    tr_z(2) top();
}

if (draw_top) { top(); }
if (draw_bottom) { bottom(); }
