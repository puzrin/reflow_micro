include <utils.scad>;

h = 1.5;
w = 2.5;
dist = 48;
screw_d = 1.8;

clones = 6;
margin = 5;

$fn = 64;

module clamp(h=10, is_last = false) {
    difference() {
        union() {
            cylinder(h=h, d=4);
            dupe_y() translate([0, dist/2, 0]) cylinder(h=h, d=4);

            linear_extrude(h) square([w, dist], center = true);

            if (!is_last) {
                // clone's connectors
                //tr_y(-0.5) linear_extrude(0.5) square([margin-1.5, 1]);
                tr_y(6) linear_extrude(0.5) square([margin, 1]);
            } else {
                // Hack to increas minimal size to order's limit (2mm)
                //tr_y(15) cylinder(h=2, d=1);
            }

        }
        translate([0, 0, -e]) cylinder(h=h+e*2, d=screw_d);
        dupe_y() translate([0, dist/2, -e]) cylinder(h=h+e*2, d=screw_d);
    }    
    
}

for(i = [1:clones]) {
    translate([margin*(i-1), 0, 0]) clamp(1.2);
}

tr_x(margin*clones*1) for(i = [1:clones]) {
    translate([margin*(i-1), 0, 0]) clamp(1.6);
}

tr_x(margin*clones*2) for(i = [1:clones]) {
    translate([margin*(i-1), 0, 0]) clamp(2.0, i == clones ? true : false);
}

module washer(h=0.5, d=6) {
    difference() {
        cylinder(h=h, d=6);
        tr_z(-e) cylinder(h=h+2*e, d=1.8);
    }
}

tr_x(-6) {
    for (i = [0:3]) tr_y(i*8) washer(0.7, 6);

    for (i = [-3:-1]) tr_y(i*8) washer(1, 6);
    
    // connectors
    for (i = [-3:2]) {
        tr_y(i*8+2) tr_x(-0.5) linear_extrude(0.4) square([1, 4]);
    }
    
    tr_y(7.5) tr_x(2) cube([4, 1, 0.4]);
}
