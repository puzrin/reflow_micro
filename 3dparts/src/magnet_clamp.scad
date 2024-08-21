include <utils.scad>;

$fn = 128;

space = 7.5;
z = 6;
x = 22;
y = 14;
w = 2;

margin = 3;
clones = 8;

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
        translate([-e, y/2, z/2]) rotate_y(90) rcube([6, space, 4], r=3);

    }

    // clones connector
    if (!last) {
        tr_y(y-e) cube([2, margin+2*e, 2]);
    }
}

for(i = [1:clones]) {
    tr_y((y+margin)*(i-1)) clamp(i == clones ? true : false);
}
