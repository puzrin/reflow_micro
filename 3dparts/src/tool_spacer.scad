include <lib/utils.scad>;

w=6;
margin = 3;
clones = 5;

module aligner (last) {
    ra_cube([1, w, 13], r = 0.5, center=false);
    ra_cube([2, w, 12], r = 0.5, center=false);
    ra_cube([3, w, 11], r = 0.5, center=false);
    ra_cube([4, w, 10], r = 0.5, center=false);
    ra_cube([5, w, 9], r = 0.5, center=false);
    ra_cube([6, w, 8], r = 0.5, center=false);

    tr_x(-10) ra_cube([16, w, 3], r = 3, center=false);

    if (!last) {
        tr_xy(4, w-1) cube([2, margin+2, 2]);
        tr_xy(-7, w-1) cube([2, margin+2, 2]);
    }
}

for (i = [1:clones]) {
    tr_y((i-1)*(w+margin)) aligner(i == clones ? true : false);
}