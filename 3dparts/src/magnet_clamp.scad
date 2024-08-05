include <utils.scad>;

$fn = 128;

space = 8;
z = 8;
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

        tr_z(-e) tr_xy(w, w) cube([x - y/2 - w, y - w*2, z + 2*e]);
        tr_z(-e) tr_xy(x - y/2, y/2) cylinder(h = z + 2*e, d = y - w*2);
        
        tr_z(-e) tr_xy(-e, (y-space)/2) cube([w + 2*e, space, z + 2*e]);
    }

    // clones connector
    if (!last) {
        tr_y(y-e) cube([2, margin+2*e, 2]);
    }
}

for(i = [1:clones]) {
    tr_y((y+margin)*(i-1)) clamp(i == clones ? true : false);
}
