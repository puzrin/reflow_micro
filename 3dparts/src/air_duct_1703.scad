include <utils.scad>;

$fn = 128;

h = 10;
wall = 1.5;
top_dia = 10;
bottom_dia = 16;

mnt_ofs = 6.75;
mnt_wall = 2;
mnt_w = 5;
mnt_hole = 1.8;

difference () {
    union () {
        hull () {
            cylinder(h = e, d = bottom_dia + wall*2);
            tr_z(h) cylinder(h = e, d = top_dia + wall*2);
        }
        
        rotate_z(45) rcube([mnt_ofs*2*1.41+mnt_w, mnt_w, mnt_wall], r=mnt_w/2);
    }

    hull () {
        tr_z(-e) cylinder(h = e, d = bottom_dia);
        tr_z(h+e) cylinder(h = e, d = top_dia);
    }
    
    tr_xy(mnt_ofs, mnt_ofs)
    union () {
        tr_z(-e) cylinder(h=mnt_wall+2e, d=mnt_hole);
        tr_z(mnt_wall) cylinder(h=10, d=3.5);
    }
    tr_xy(-mnt_ofs, -mnt_ofs)
    union () {
        tr_z(-e) cylinder(h=mnt_wall+2e, d=mnt_hole);
        tr_z(mnt_wall) cylinder(h=10, d=3.5);
    }
    
}