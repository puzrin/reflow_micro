include <lib/utils.scad>;

$fn = 64;

w = 25;
l = 50;
h = 25;
wall = 3;
leg_h = 2;
prism_point = 6;
sqrt2 = sqrt(2);

linear_extrude(wall)
polygon([
    [prism_point, 0],
    [0, prism_point],
    [0, w/2],
    [l, w/2],
    [l, -w/2],
    [0, -w/2],
    [0, -prism_point]
]);

linear_extrude(h - leg_h)
polygon([
    [prism_point, 0],
    [0, prism_point],
    [wall/sqrt2, prism_point + wall/sqrt2],
    [prism_point + wall*sqrt2, 0],
    [wall/sqrt2, -(prism_point + wall/sqrt2)],
    [0, -prism_point],
]);

// Legs
dupe_y()
tr_z(-leg_h+e)
tr_y(w/2 - wall)
cube([l, wall, leg_h]);
