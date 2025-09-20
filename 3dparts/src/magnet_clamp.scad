include <lib/utils.scad>;

$fn = $preview ? 16 : 64;
$ra_fn = $preview ? 3 : 16;

space = 7.5;
z = 6;
x = 22;
y = 14;
w = 2;

margin = 3;
clones = 10;

module clamp (last) {
    difference () {
        union () {
            cube([x - y/2, y, z]);
            tr_xy(x - y/2, y/2) cylinder(h = z, d = y);
        }

        // Inner
        tr_z(-e) tr_xy(w, w) cube([x - y/2 - w, y - w*2, z + 2*e]);
        tr_z(-e) tr_xy(x - y/2, y/2) cylinder(h = z + 2*e, d = y - w*2);

        // clamp space
        tr_z(-e) tr_xy(-e, (y-(space-1.5))/2) cube([w + 2*e, space-1.5, z + 2*e]);
        translate([-e, y/2, z/2]) rotate_y(90) ra_cube([6, space, 4], r=3, fn=$ra_fn);

    }

    // clones connector
    if (!last) {
        //tr_y(y-e) cube([2, margin+2*e, 2]);
        cube([2, 2, z+margin+1]);
        tr_y(y-2) cube([2, 2, z+margin+1]);
        tr_xy(x-2, y/2-1) cube([2, 2, z+margin+1]);
    }
}

for(i = [1:clones]) {
    tr_x((-z-margin)*(i-1)) rotate_y(-90) clamp(i == clones ? true : false);
}
