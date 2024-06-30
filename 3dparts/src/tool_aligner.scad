include <utils.scad>;

$fn = 64;

w=8;

cube([15, 3, w]);

tr_y(-15) cube([3, 30, w]);

tr_y(0.5) tr_x(-0.8+e) cube([0.8, 3-0.5-1, w]);