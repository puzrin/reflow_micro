include <fan_pi5_data.scad>;
include <lib/utils.scad>;
include <lib/nut_trap.scad>;

$fn = $preview ? 16 : 64;
$ra_fn = $preview ? 3 : 16;

make_debug_hole = 0;

cap_screw_hole_d = 3.3;
cap_screw_hole_selftap = 2.6;

cap_screw_axis_offset = 3.15;
cap_screw_mount_h = 6;
cap_screw_head_h = 1.7 // screw head height
    + (4 -1.6) // D4L4 Magnet outset over tray case
    + 0.5; // Extra space

// PCB mount holes
pcb_mount_x_offset = 3.0;
pcb_mount_y_offset = 25;
pcb_mount_zw = 3.0;

// !!! Measure magnets height before case order.
// Magnets from Aliexpress can be smaller than declared size
magnet_d = 6;
magnet_h = 6.1 + 0.1; // D3L3 x 2 - a bit more than 6.0, + margin
magnet_hole_d = magnet_d + 0.3;
magnet_wall = 1.5;
magnet_axis_offset = 3.15;

wall_hor = 2;
wall_side = 1.75;

pcb_wx = 90;
pcb_wy = 80;
pcb_r= 3;
pcb_support_w = 0.5;
pcb_side_margin = 0.1;
pcb_top_margin = 0.1; // extra space for better cap close
// 1.6 PCB + margin
pcb_h  = 1.6 + pcb_top_margin;

case_wx = pcb_wx + 2*wall_side;
case_wy = pcb_wy + 2*wall_side;

case_locker = 1; // ring size to position cap to tray

case_r_vert = pcb_r + wall_side;
case_r_bottom = 2;
case_r_top = 2;

tray_inner_h = 8; //6.7;
tray_h  = tray_inner_h + wall_hor + pcb_h;

// plate (2.8) + space (10) + reflector (1.6) + space (4) + shield (1.2)
// + space(5) + cap(1.6) + extra (1)
cap_inner_h = 27.5;

cap_h = cap_inner_h + wall_hor;
cap_pcb_margin = 0.1;

cap_stiffener_h = cap_inner_h - 14;
cap_stiffener_w = 4.4;

// Set btn height to make hole h = 3mm. For easy cleanup with drill bit.
btn_h = 3.3; // 3.5mm - 2*margin
btn_w = 9.8;
btn_margin = 0.1;
btn_marks = 1; // to visually distinguish sizes
btn_protrusion = 1.5;

btn_pusher_w = 12;
btn_pusher_base_w = 1.5;
btn_pusher_pcb_depth = 6;

led_h = 0.85; // 0.5...0.6 for different models

module case_left(ofs = 0) { tr_x(ofs - case_wx/2) children(); }
module case_right(ofs = 0) { tr_x(ofs + case_wx/2) children(); }
module case_front(ofs = 0) { tr_y(ofs - case_wy/2) children(); }
module case_back(ofs = 0) { tr_y(ofs + case_wy/2) children(); }

// esp32 - 3.2mm, USB - 3.26mm
assert(tray_inner_h > 3.5, "Not enough room for PCB components");

module quoter_cylinder(r=10, h=30, fn=16) {
    ra_cube([r, r, h], r=[0, 0, r, 0], center=false, fn=fn);
}

module quoter_round_wall(r_out=10, r_in=8, h=30, fn=16) {
    polyRoundExtrude(
        [
            [0, r_out, 0],
            [r_out, r_out, r_out],
            [r_out, 0, 0],
            [r_in, 0, 0],
            [r_in, r_in, r_in],
            [0, r_in, 0]
        ],
        h, 0, 0, fn
    );
}

module usb_hole() {
    // Connector sizes
    usb_w = 9.0;    // 8.94 by doc, 8.97 real
    usb_h = 3.2;    // 3.26 by doc, ;   //

    w = usb_w + 0.5;
    h = usb_h + 0.5;
    r = 1.6;

    tr_z(tray_h-pcb_h-usb_h/2 + 0.05)
    tr_y(pcb_wy/2 - pcb_support_w -e)
    rotate_x (-90) {
        ra_cube([w, h, 10], r=r, fn=$ra_fn);

        tr_y(-h/2)
        linear_extrude(pcb_support_w + pcb_side_margin + e)
        square([w, h], center=true);
    }
}

module usb_debug_hole() {
    w = 12.0;
    h = 7.0;
    r = 3;

    tr_z(tray_h - h)
    tr_y(-(pcb_wy/2-3))
    tr_x(58.5-45)
    rotate_x(90)
    tr_x(-w/2)
    ra_cube([w, h+10, 10], r=r, center = false, fn=$ra_fn);

}

module tray_pcb_holders() {
    r = 4/2 + 1.7;
    h = tray_inner_h; // - 0.3;

    dupe_xy()
    tr_xy(-pcb_wx/2 + pcb_mount_x_offset, pcb_mount_y_offset)
    tr_z(wall_hor-e)
    difference() {
        ra_cube([2*r, 2*r, h], r=[0, 0, r, r], center = true, fn=$ra_fn);

        tr_z(e) cylinder(h, d=2.5); // screw hole
        tr_y(-2.5/2) cube([10, 2.5, h - pcb_mount_zw]); // dust remove channel
        tr_z(h - pcb_mount_zw) mirror_z() nut_trap(h=1.6, side_depth=20, clearance=0.05); // nut trap
        // extra nut indentation, to hold in place after insert
        tr_z(h - pcb_mount_zw - 0.6) mirror_z() nut_trap(h=1.6, clearance=0.05);
    }
}

module magnet_supports_add() {
    dupe_xy()
    tr_z(wall_hor-e)
    tr_xy(-pcb_wx/2 - e, -pcb_wy/2 - e)
    ra_cube(
        [magnet_hole_d + magnet_wall, magnet_hole_d + magnet_wall, tray_inner_h],
        r = [pcb_r, 0, magnet_hole_d/2 + magnet_wall, 0],
        rbottom = -1,
        center = false
    );
}

module magnet_supports_remove() {
    dupe_xy()
    tr_z(tray_inner_h + wall_hor + e)
    tr_xy(-pcb_wx/2 + magnet_axis_offset, -pcb_wy/2 + magnet_axis_offset)
    mirror_z() {
        cylinder(h = magnet_h, d = magnet_hole_d);

        rotate_z(45)
        tr_x(magnet_hole_d/2) linear_extrude(magnet_h) square([magnet_hole_d, 3.5], center=true);
    }
}

module cap_locks(h = 20) {
    h = cap_inner_h - cap_screw_head_h;

    dupe_xy()
    tr_z(wall_hor-e)
    tr_xy(-pcb_wx/2 - e, -pcb_wy/2 - e)
    difference() {
        union() {
            // screw mounting
            tr_z(h -  cap_screw_mount_h)
            tr_xy(-1, -1)
            ra_cube(
                [7 + 1, 7 + 1, cap_screw_mount_h],
                r = [pcb_r, 0, 0, 0],
                center = false
            );

            // guide
            tr_xy(8, 8)
            cylinder(h = h, r = 5.35);
        }

        // screw hole
        tr_xy(cap_screw_axis_offset, cap_screw_axis_offset)
        union() {
            cylinder(h=cap_inner_h, d=cap_screw_hole_d);
            tr_z(h + e) mirror_z()
                cylinder(h=0.4, d1=cap_screw_hole_d+0.4*2, d2=cap_screw_hole_d);
        }

        // walls
        translate([8, 8, -e])
        union() {
            cylinder(h = h + 2*e, r = 3.75);
            tr_z(h + 2*e) mirror_z()
                cylinder(h = 0.5, r1 = 3.75 + 0.5, r2 = 3.75);
        }

        translate([7, 0, -e]) cube([50, 50, 50]);
        translate([0, 7, -e]) cube([50, 50, 50]);
    }
}

module stiffener_x(len=100, center=true) {
    fillet = 0.5;
    w = 2;
    l = len + 2*e;
    center_offs = center ? -l/2 : 0;

    rotate_z(90)
    tr_z(wall_hor-e)
    rotate_x(90)
    tr_z(center_offs)
    linear_extrude(l)
    polygon([
        [-w/2 - fillet, 0], [-w/2, fillet], [-w/2, w - fillet], [-w/2 + fillet, w],
        [w/2 - fillet, w], [w/2, w - fillet], [w/2, fillet], [w/2 + fillet, 0]
    ]);
}

module stiffener_y(len=100, center=true) {
    rotate_z(90)
    stiffener_x(len, center);
}

module _tray_base() {
    difference() {
        ra_cube([case_wx, case_wy, tray_h], case_r_vert, 0, case_r_bottom, fn=$ra_fn);

        // Inner
        tr_z(wall_hor)
        ra_cube([
            case_wx-2*wall_side-2*pcb_support_w,
            case_wy-2*wall_side-2*pcb_support_w,
            tray_h-wall_hor+e
        ], magnet_d/2, 0, 0.7, fn=$ra_fn);

        // PCB placement
        tr_z(tray_h-pcb_h)
        ra_cube([
            case_wx-2*wall_side+2*pcb_side_margin,
            case_wy-2*wall_side+2*pcb_side_margin,
            pcb_h+e
        ], r=pcb_r+pcb_side_margin, fn=$ra_fn);

        // USB connector
        usb_hole();

        // Button hole
        case_front(wall_side+pcb_support_w+e) tr_z(tray_h/2)
        rotate_x(90)
        ra_cube(
            [btn_w + 2*btn_margin, btn_h + 2*btn_margin, wall_side+pcb_support_w+2*e],
            r=btn_h/2+btn_margin, fn=$ra_fn
        );

        // Bottom heels
        dupe_xy() tr_xy(case_wx/2 - 7.5, case_wy/2 - 7.5) tr_z(-e) cylinder(h=0.6, d=6.2);

        // Extra space for fan mounting
        /*for (i = fan_mount_coords) {
            tr_xy(5, -15)
            translate(i)
            tr_z(wall_hor+e) mirror_z() {
                cylinder(h=0.5, d=4.5);
                //cylinder(h=1.1, d=4.5);
            }
        }*/

        // Partially remove stiffeners for the Pi 5 fan connector
        //tr_xy(-pcb_wx/2 + 68.5, -pcb_wy/2 + 21) tr_z(wall_hor) cylinder(h=3, r=7);
    }
}


module tray() {
    difference () {
        union () {
            _tray_base();

            magnet_supports_add();
            tray_pcb_holders();

            // PCB supports
            dupe_xy()
            tr_xy(7, -pcb_wy/2) tr_z(wall_hor--e) cube([1.5, 1.5, tray_inner_h]);

            // Button guides
            dupe_x()
            tr_z(wall_hor-e)
            tr_x(btn_pusher_w/2 + 0.25)
            tr_y(-pcb_wy/2 + btn_pusher_pcb_depth + 1)
            mirror_y() cube([2, 4, 1.7]);

            // Stiffeners
            /*mirror_x() tr_x(9) stiffener_x(pcb_wx/2 - 9, false);
            dupe_y() tr_y(20) stiffener_x(pcb_wx);

            tr_x(-9) stiffener_y(pcb_wy);
            tr_x(-27) stiffener_y(pcb_wy);
            dupe_y() {
                tr_xy(9, 20) stiffener_y(pcb_wy/2-20, false);
                tr_xy(27, 20) stiffener_y(pcb_wy/2-20, false);
            }*/
        }

        magnet_supports_remove();
        if (make_debug_hole) {
            usb_debug_hole();
        }
    }
}

module cap_stiffener_x() {
    h = cap_stiffener_h;
    w = cap_stiffener_w;

    tr_y(-pcb_wy/2 - e)
    tr_z(wall_hor-e) tr_x(1)
    rotate_y(-90)
    linear_extrude(2)
    polygon([[0, 0], [h, 0], [h - w, w], [0, w]]);
}

module cap_stiffener_y() {
    h = cap_stiffener_h;
    w = cap_stiffener_w;

    tr_x(pcb_wx/2 + e)
    rotate_z(90)
    tr_z(wall_hor-e) tr_x(1)
    rotate_y(-90)
    linear_extrude(2)
    polygon([[0, 0], [h, 0], [h - w, w], [0, w]]);
}

module cap() {
    union() {
        difference() {
            ra_cube([case_wx, case_wy, cap_h],
                case_r_vert, 0, case_r_bottom, fn=$ra_fn);

            // Inner
            tr_z(wall_hor)
            ra_cube([pcb_wx, pcb_wy, cap_h],
                magnet_d/2, 0, 0.7, fn=$ra_fn);

            // PCB margin
            tr_z(cap_h - cap_screw_head_h + e)
            ra_cube([pcb_wx+cap_pcb_margin*2, pcb_wy+cap_pcb_margin*2, cap_h],
                magnet_d/2, 0, 0, fn=$ra_fn);

            // Small cone conductors for alu cover
            gap = 0.6;

            tr_z(cap_h + e)
            hull() {
                ra_cube([pcb_wx + gap*2, pcb_wy + gap*2, 1], pcb_r + gap, 0, 0, fn=$ra_fn);
                tr_z(-gap)
                    ra_cube([pcb_wx, pcb_wy, e], pcb_r, 0, 0, fn=$ra_fn);
            }
            /*tr_z(cap_h+e)
            mirror_z()
            linear_extrude(gap, scale=[pcb_wx/(pcb_wx + gap*2), 1])
            square([pcb_wx + gap*2, pcb_wy-2*(magnet_d/2+pcb_support_w)], center=true);

            tr_z(cap_h+e)
            mirror_z()
            linear_extrude(gap, scale=[1, pcb_wy/(pcb_wy + gap*2)])
            square([pcb_wx-2*(magnet_d/2+pcb_support_w), pcb_wy + gap*2], center=true);*/

            tr_z(-e)
            rotate_z(90)
            mirror_x()
            logo();
        }

        cap_locks(cap_inner_h);

        // Stiffeneres
        dupe_y() {
            cap_stiffener_x();
            dupe_x() tr_x(22) cap_stiffener_x();
        }

        dupe_xy() tr_y(12) cap_stiffener_y();
    }
}

module button() {
    btn_middle_h = tray_h/2 - wall_hor;
    btn_inner_depth = btn_pusher_pcb_depth - pcb_support_w;
    // Calculate back size to leave 0.5mm to led
    btn_back_h = (tray_inner_h - led_h - btn_middle_h - 0.5)*2;

    // "ring" should be > 0.3mm
    assert(btn_back_h - btn_h > 0.3*2, "Too few space for button");

    // temporary guides
    //tr_z(tray_inner_h) linear_extrude(0.1) square(10, center=true);
    //tr_z(tray_inner_h-led_h) linear_extrude(0.1) square(10, center=true);
    //tr_z(-0.1) linear_extrude(0.1) square(10, center=true);

    //tr_z(btn_middle_h) rotate_x(-90)
    difference () {
        union () {
            // Front part, 1mm out of case
            tr_z(btn_inner_depth - e)
            ra_cube([btn_w, btn_h, wall_side + pcb_support_w + btn_protrusion],
                btn_h/2, .5, 0, fn=$ra_fn);

            // Inner
            ra_cube([btn_pusher_w, btn_back_h, btn_inner_depth],
                1.0, 0, 0, fn=$ra_fn);

            // Pusher
            p_space = 0.3; // total top/bottom margin, 0.15mm each,
            p_h = tray_inner_h - p_space;

            tr_y(-tray_inner_h/2 + btn_middle_h)
            ra_cube([btn_pusher_w, p_h, btn_pusher_base_w], 1.0, fn=$ra_fn);
        }

        // Light mirror
        mside = btn_back_h - 1;
        m_w = btn_pusher_w - 3;

        tr_z(btn_pusher_base_w)
        tr_xy(m_w/2, btn_back_h/2+e) rotate_y(-90)
        linear_extrude(m_w)
        polygon([[0, 0], [mside, 0], [0, -mside]]);

        // Led diffuser
        led_offset = 2.5 - pcb_support_w; // Offset from wall
        translate([0, -btn_back_h/2, btn_inner_depth-2])
        sphere(d=2);

        // Marks
        if (btn_marks > 0) {
            for(i = [1:btn_marks]) {
                tr_x((i-1)*2.5 - (btn_marks-1)*2.5/2) tr_z(-e) cylinder(h=0.5, d=1);
            }
        }
    }
}

module logo() {
    w = 16.58;
    h = 20.7;
    line_w = 1.75;

    line_sp = (h - line_w*5) / 4;
    r_out = (line_sp + line_w*2) / 2;
    r_in = line_sp / 2;

    radii_points = [
        [0, 0, line_w/2],
        [0, line_w, line_w/2],
        [w - line_w, line_w, r_in],
        [w - line_w, line_w + line_sp, r_in],
        [0, line_w + line_sp, r_out],
        [0, line_w*3 + line_sp*2, r_out],
        [w - line_w, line_w*3 + line_sp*2, r_in],
        [w - line_w, line_w*3 + line_sp*3, r_in],
        [0, line_w*3 + line_sp*3, r_out],
        [0, line_w*5 + line_sp*4, r_out],
        [w, line_w*5 + line_sp*4, line_w/2],
        [w, line_w*4 + line_sp*4, line_w/2],
        [line_w, line_w*4 + line_sp*4, r_in],
        [line_w, line_w*4 + line_sp*3, r_in],
        [w, line_w*4 + line_sp*3, r_out],
        [w, line_w*2 + line_sp*2, r_out],
        [line_w, line_w*2 + line_sp*2, r_in],
        [line_w, line_w*2 + line_sp, r_in],
        [w, line_w*2 + line_sp, r_out],
        [w, 0, r_out]
    ];

    tr_xy(-w/2, -h/2)
    polyRoundExtrude(radii_points, 0.5, 0.2, -0.2, $ra_fn);
}

DRAW_TRAY = 0;
DRAW_CAP = 0;
DRAW_BTN = 0;

if (!DRAW_TRAY && !DRAW_CAP && !DRAW_BTN) {
    tray();
    tr_x(100) cap();
    tr_x(-70) button();
}
else {
    if (DRAW_TRAY) { tray(); }
    if (DRAW_CAP) { tr_x(100) cap(); }
    if (DRAW_BTN) { tr_x(-70) button(); }
}
