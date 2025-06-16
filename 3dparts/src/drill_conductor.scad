include <utils.scad>;

$fn = 64;

w = 25;
l = 50;
d_guide = 7;
r_guide = d_guide/2;

wall = 3;
space = 10;

difference () {
    union () {
        // body
        linear_extrude(space + wall)
        polygon([
            [w/2, 0],
            [w/2, -l],
            [-w/2, -l],
            [-w/2, 0],
            //[0, w/2]
            [-r_guide/sqrt(2), w/2 - r_guide/sqrt(2)],
            [r_guide/sqrt(2), w/2 - r_guide/sqrt(2)]
        ]);
        
        // drill guide
        tr_y(w/2 - r_guide*sqrt(2))
        tr_z(space)
        cylinder(h=20+ wall, r=r_guide);
    }

    // Inner main
    tr_z(-e) tr_y(-e - 2)
    linear_extrude(space)
    polygon([
        [w/2 - wall, 0],
        [w/2 - wall, -l],
        [-w/2 + wall, -l],
        [-w/2 + wall, 0],
        [0, w/2 - wall]
    ]);
    
    // Nose gap
    tr_z(-e)
    tr_x(-10/2)
    cube([10,50, space]);
    
    // drill guide hole
    tr_y(w/2 - r_guide*sqrt(2))
    cylinder(h=100, d=3.3);
};