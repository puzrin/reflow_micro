include <lib/utils.scad>;

$fn = 64;

w=6;
margin = 3;
clones = 5;

module aligner (last) {
    rcube([1, w, 13], r = 0.5, center=false);
    rcube([2, w, 12], r = 0.5, center=false);
    rcube([3, w, 11], r = 0.5, center=false);
    rcube([4, w, 10], r = 0.5, center=false);
    rcube([5, w, 9], r = 0.5, center=false);
    rcube([6, w, 8], r = 0.5, center=false);

    tr_x(-10) rcube([16, w, 3], r = 3, center=false);

    if (!last) {
        tr_xy(4, w-1) cube([2, margin+2, 2]);
    }
}

for (i = [1:clones]) {
    tr_y((i-1)*(w+margin)) aligner(i == clones ? true : false);
}