include <../../src/lib/utils.scad>;

$fn = $preview ? 16 : 64;
$ra_fn = $preview ? 3 : 8;

mch_count = 2;
clamp_count = 6;

assert(mch_count == 2 || mch_count == 3,
    str("mch_count must be 2 or 3, got ", mch_count));

h = 1.5;
w_bar = 2.5;
w_plate = 5.0;
d_hole = 2.0;
r_plate = 1.0;
r_champfer = 0.3;

frame_wall = 3;
frame_x_space = 1;
frame_y_space = 3;

edge_holes_dist = mch_count == 2 ? 48 : 60;
plate_y_positions = mch_count == 2 ?
    [-edge_holes_dist/2, 0, edge_holes_dist/2] :
    [-edge_holes_dist/2, -edge_holes_dist/2/3, edge_holes_dist/2/3, edge_holes_dist/2];

i_frame_wx = (w_plate + frame_x_space) * clamp_count + frame_x_space;
i_frame_wy = edge_holes_dist + w_plate + 2*frame_y_space;

frame_wx = i_frame_wx + 2*frame_wall;
frame_wy = i_frame_wy + 2*frame_wall;

module plate_body() {
    ra_cube([w_plate, w_plate, h], r=r_plate, rtop=r_champfer, rbottom=r_champfer, fn=$ra_fn);
}

module plate_hole() {
    ra_cube([d_hole, d_hole, h + 2*e], r=d_hole/2, rtop=-0.2, rbottom=-0.2, fn=$ra_fn);
}

module bar() {
    ra_cube(
        [w_bar, i_frame_wy + 2, h],
        r=0, rtop=r_champfer, rbottom=r_champfer, fn=$ra_fn);
}

module clamp() {
    difference() {
        union() {
            bar();
            for (y = plate_y_positions) tr_y(y) plate_body();
        }
        // Holes
        for (y = plate_y_positions) translate([0, y, -e]) plate_hole();
    }
}

module frame() {
    difference() {
        tr_xy(-frame_wall, -frame_wall)
        ra_cube([frame_wx, frame_wy, frame_wall],
            r=frame_wall, rtop=r_champfer, rbottom=r_champfer,
            center=false, fn=$ra_fn);

        tr_z(-e)
        ra_cube([i_frame_wx, i_frame_wy, frame_wall + 2*e],
            r=frame_wall/2, rtop=-r_champfer, rbottom=-r_champfer,
            center=false, fn=$ra_fn);
    }
}

for (i = [0 : clamp_count - 1]) {
    tr_x(i * (w_plate + frame_x_space)) clamp();
}

tr_xy(-w_plate/2 - frame_x_space, -i_frame_wy/2)
frame();
