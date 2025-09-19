include <lib/utils.scad>;

$fn = 64;
wall = 2;
r = 2.8;
w = 10;
pcb_h = 1.6;
pcb_space = pcb_h + 0.3;
base_h = 13;
latch = 0.3; // Update if needed
latch_w = 3;
margin = 5;
clones = 5;

back_h = pcb_space + (wall-0.5) ;

module base(xy, h, r) {
    linear_extrude(h)
    hull () {
        tr_xy (r, r) circle(r);
        tr_y(xy) circle(e);
        tr_xy (xy-r, xy-r) circle(r);
        tr_x(xy) circle(e);
    }
}

module aligner (last) {
    difference () {
        union () {
            tr_z(-back_h) tr_xy(-wall, -wall) base(xy = w + wall, h = base_h + back_h, r = r + wall);
            base(xy = w, h = base_h, r = r);
        }
        tr_z(wall) tr_xy(0.5, 0.5) base(xy = w, h = base_h, r = r);
        tr_z(-pcb_space) base(xy = w, h = pcb_space, r = r);

        tr_xy(-2.5, -2.5)
        tr_z(-pcb_space+e)
        rotate(45) mirror_z() linear_extrude(5) square(w*sqrt(2), center=true);

        tr_xy(11, 11)
        tr_z(-pcb_space+e)
        rotate(45) mirror_z() linear_extrude(5) square(w*sqrt(2), center=true);

    }

    // latch
    tr_z(-pcb_h + latch)
    tr_xy(w/2, w/2)
    tr_z(-1) rotate_z(45) tr_y(latch_w/2) rotate_x(90)
    linear_extrude(latch_w) polygon([[-1.4,0.2], [0,1], [1.4,0.2], [1.4,-1], [-1.4,-1]]);

    // clones connector
    if (!last) {
        tr_xy(w-e, -wall) cube([margin+4, 2, 2]);
    }
}

rotate_x(90) {
    for (i = [1:clones]) {
        tr_x((i-1)*(w+wall+margin)) aligner(i == clones ? true : false);
    }
}
