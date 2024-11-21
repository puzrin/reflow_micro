include <utils.scad>;

h = 1.1;
w = 2.5; // bridge between holes
dist = 48; // space between edge holes
screw_d = 1.8;
screw_support_d = 4.5;

frame_space = 3;
bridge_w = 1.5;
bridge_h = 0.5;
clamp_space = 1.5;
clones = 10;
washer_clones = clones/2;

frame_height = 3;
frame_wall = 2;

washer_wx = 4.5;
washer_wy = 7.5;
washer_h = 0.7;
washer_pin_wx = 2.1;
washer_pin_h = 1.1;

$fn = 64;

// Intermadiate vars
clamp_wy = dist + screw_support_d;
clamp_wx = screw_support_d;

frame_clamps_wx = clones*clamp_wx + (clones+1)*clamp_space;
frame_wy = clamp_wy + frame_space*2;

frame_washer_wx = washer_wx + frame_space*2;

frame_all_wx = frame_clamps_wx + frame_wall + frame_washer_wx;


module clamp() {
    difference() {
        union() {
            // Screw supports
            cylinder(h=h, d=screw_support_d);
            dupe_y() translate([0, dist/2, 0]) cylinder(h=h, d=screw_support_d);

            // Body
            linear_extrude(h) square([w, dist], center = true);

            // Top/bottom bridges
            linear_extrude(bridge_h) square([bridge_w, frame_wy+2*e], center=true);
            //linear_extrude(bridge_h) square([7, 1], center=true);
        }
        // Holes
        translate([0, 0, -e]) cylinder(h=h+e*2, d=screw_d);
        dupe_y() translate([0, dist/2, -e]) cylinder(h=h+e*2, d=screw_d);
    }

}

module washer() {
    difference() {
        union() {
            // Central part
            //rcube([washer_wx, washer_wy, washer_h], r=0.5);
            cylinder(h=washer_h, d=washer_wx);
            rcube([2.5, washer_wy, washer_h], r=0.3);
            // Pin
            translate([-washer_pin_wx/2, -washer_wy/2, washer_h - e])
            rcube([washer_pin_wx, 1, washer_pin_h], r=0.2, center=false);

            // Side bridges
            rcube([washer_wx + 2*frame_space + 2*e, 1.5, 0.5], r=0);
        }
        // Holes
        tr_z(-e) cylinder(h=h+e*2, d=screw_d);
    }
}


// Frame

difference() {
    translate([-frame_wall, -frame_wall, 0])
    rcube([
        frame_all_wx + frame_wall*2,
        frame_wy + frame_wall*2,
        frame_height
    ], center=false, r=2);

    // Clamps inner
    tr_z(-e)
    rcube([frame_clamps_wx, frame_wy, frame_height+2*e], center=false);

    // Washers inner
    translate([frame_clamps_wx + frame_wall, 0, -e])
    rcube([frame_washer_wx, frame_wy, frame_height+2*e], center=false);

}

// Clamps

for(i = [1:clones]) {
    translate([(clamp_wx + clamp_space)*(i-1), 0, 0])
    translate([clamp_wx/2 + clamp_space, frame_wy/2, 0]) clamp();
}

// Washers
step = frame_wy / (washer_clones + 1);
for(i = [1:washer_clones]) {
    translate([frame_clamps_wx + frame_wall + frame_washer_wx/2, step * i, 0])
    washer();
}

/*translate([frame_clamps_wx + frame_wall + frame_washer_wx/2, frame_wy/2, 0]) {
    tr_y(frame_wy/10) washer();
    tr_y(frame_wy/10*3) washer();
    tr_y(-frame_wy/10) washer();
    tr_y(-frame_wy/10*3) washer();
}*/