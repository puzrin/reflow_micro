include <utils.scad>;

h = 1.5;
w = 2.5; // bridge between holes
dist = 48; // space between edge holes
screw_d = 1.8;
screw_support_d = 4.5;

frame_space = 3;
bridge_w = 1.5;
bridge_h = 1.5;
clamp_space = 1.5;
clones = 6;
washer_clones = 5;

frame_height = 3;
frame_wall = 3;

washer_wx = 4.5;
washer_wy = 7.5;
washer_h = 1.5;
washer_pin_wx = 2.1;
washer_pin_h = 1.1;

$fn = 64;

// Intermadiate vars
clamp_wy = dist + screw_support_d;
clamp_wx = screw_support_d;

frame_clamps_wx = clones*w + (clones+1)*frame_space;
frame_wy = clamp_wy + frame_space*2;

frame_washer_wx = washer_wx + frame_space*2;

frame_all_wx = frame_clamps_wx + frame_wall + frame_washer_wx;


module clamp(y_bridges=1) {
    difference() {
        union() {
            // Screw supports
            //cylinder(h=h, d=screw_support_d);
            //dupe_y() translate([0, dist/2, 0]) cylinder(h=h, d=screw_support_d);
            rcube([screw_support_d, screw_support_d, h], r=1);
            dupe_y() tr_y(dist/2) rcube([screw_support_d, screw_support_d, h], r=1);
            
            // Body
            linear_extrude(h) square([w, dist], center = true);

            // Top/bottom bridges
            if (y_bridges) {
                linear_extrude(bridge_h) square([bridge_w, frame_wy+2*e], center=true);
            }
            //linear_extrude(bridge_h) square([7, 1], center=true);
        }
        // Holes
        tr_z(-e) cylinder(h=h+e*2, d=screw_d);
        dupe_y() translate([0, dist/2, -e]) cylinder(h=h+e*2, d=screw_d);
    }

}

module washer() {
    difference() {
        union() {
            // Central part
            //rcube([washer_wx, washer_wy, washer_h], r=0.5);
            cylinder(h=washer_h, d=washer_wx);
            rcube([washer_wx, washer_wx, washer_h], r=1);
            rcube([2.5, washer_wy, washer_h], r=0.3);
            // Pin
            translate([-washer_pin_wx/2, -washer_wy/2, washer_h - e])
            rcube([washer_pin_wx, 1, washer_pin_h], r=0.2, center=false);

            // Side bridges
            rcube([washer_wx + 2*frame_space + 2*e, bridge_w, bridge_h], r=0);
            // Bottom bridge
            tr_y(-frame_space/2 - washer_wy/2)
            rcube([bridge_w, frame_space + 2*e, bridge_h], r=0);
        }
        // Holes
        tr_z(-e) cylinder(h=h+e*2, d=screw_d);
    }
}


// Frame

difference() {
    tr_xy(-frame_wall, -frame_wall)
    union() {
        rcube([
            frame_all_wx + frame_wall*2,
            frame_wy + frame_wall*2,
            frame_height
        ], center=false, r=2);
        // Extra reinforcement
        /*tr_xy(-2, -2)
        rcube([
            frame_all_wx + frame_wall*2 + 4,
            frame_wy + frame_wall*2 + 4,
            3
        ], center=false, r=2);*/
    }
    // Clamps inner
    tr_z(-e)
    rcube([frame_clamps_wx, frame_wy, frame_height+2*e], center=false);

    // Washers inner
    translate([frame_clamps_wx + frame_wall, 0, -e])
    rcube([frame_washer_wx, frame_wy, frame_height+2*e], center=false);

}

// Clamps

for(i = [1:clones]) {
    y_bridges = i == 3 || i == 4;
    offset = frame_space + w/2;
    step = frame_space + w;

    tr_x(offset + step*(i-1))
    tr_y(frame_wy/2) clamp(y_bridges);
}
// supports
tr_y(frame_wy/2)
dupe_y()
tr_xy(frame_clamps_wx/2, dist/4)
rcube([frame_clamps_wx + 2*e, bridge_w, bridge_h], r=0);


// Washers
for(i = [1:washer_clones]) {
    offset = frame_space + washer_wy/2;
    step = frame_space + washer_wy;
    
    translate([
        frame_clamps_wx + frame_wall + frame_washer_wx/2,
        offset + step*(i-1),
        0
    ])
    washer();
}
