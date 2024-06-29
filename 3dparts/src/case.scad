include <utils.scad>;

$fn = $preview ? 16 : 64;

// Inserts size for PCB mount (M1.6)
insert_d = 2.5 + 0.1;
insert_h = 4 + 0.3;
insert_pcb_x_offset = 2.5;
insert_pcb_y_offset = 25;

// !!! Measure magnets height before case order.
// Magnets from Aliexpress can be smaller than declared size
MAGNET_H = 6;
magnet_d = 6;
magnet_h = MAGNET_H;
magnet_margin_h = 0.1;

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
case_r_bottom = 1;
case_r_top = 1;

tray_inner_h = 6.7;
tray_h  = tray_inner_h + wall_hor + pcb_h;

// plate (4) + space (12) + reflector (1.2/1.6) + space (4) + shield (1.2) + extra (1)
cap_inner_h = 4 + 12 + 1.6 + 4 + 1.2 + 2;

cap_h = cap_inner_h + wall_hor;

btn_h = 3;
btn_w = 8;
btn_margin = 0.1;

btn_pusher_w = 10;
btn_pusher_base_w = 1.5;
btn_pusher_pcb_depth = 6;
//btn_ring_w = 0.4;

led_h = 1.1; // 0.8...1.1 for different models

module case_left(ofs = 0) { tr_x(ofs - case_wx/2) children(); }
module case_right(ofs = 0) { tr_x(ofs + case_wx/2) children(); }
module case_front(ofs = 0) { tr_y(ofs - case_wy/2) children(); }
module case_back(ofs = 0) { tr_y(ofs + case_wy/2) children(); }

// esp32 - 3.2mm, USB - 3.26mm
assert(tray_inner_h > 3.5, "Not enougth room for PCB components");

module usb_hole() {
    // outer sizes
    usb_w = 8.94;
    usb_h = 3.26;
    usb_r = 1.52;

    tr_z(-usb_h/2)
    rotate_x (-90) {
        tr_z(-e) rcube([usb_w, usb_h, 10], r=usb_r);

        w = usb_w + 0.2;
        h = usb_h + 0.2;
        mirror_z() rcube([w, h, 10], r=usb_r);
        mirror_y() mirror_z() tr_x(-w/2) linear_extrude(10) square(w, 10);
    }
}

module tray_pcb_holder() {
    r = insert_d/2 + 1.2;
    h = tray_inner_h; // - 0.3;

    tr_z(wall_hor-e)
    difference() {
        linear_extrude(h)
        rsquare([2*r, 2*r], r=[0, 0, r, r], center = true);

        tr_z(h+e) mirror_z() cylinder(h=insert_h, r=insert_d/2);
    }
}

module magnet_support(h = 20) {
    r = magnet_d/2;
    reserve_w = 1;
    mh = magnet_h + magnet_margin_h;
    assert(mh <= tray_inner_h, "Tray inner too small for desired magnet height")

    tr_xy(-3.5, -3.5)
    difference() {
        union() {
            cylinder(h = h, r = r);
            rotate_extrude(angle=90) square([r+reserve_w, h]);
        }

        tr_z(h - mh + e) cylinder(h = mh, r = r+e);
    }
}

module magnet_support_4x(h=20) {
    dupe_xy()
    tr_xy(pcb_wx/2, pcb_wy/2) tr_z(wall_hor-e) magnet_support(h+e);
}

module _tray_base() {
    difference() {
        rcube([case_wx, case_wy, tray_h], r = case_r_vert, rbottom = case_r_bottom);

        // Inner
        tr_z(wall_hor)
        rcube(
            [case_wx-2*wall_side-2*pcb_support_w, case_wy-2*wall_side-2*pcb_support_w, tray_h-wall_hor+e],
            r = magnet_d/2
        );

        // PCB placement
        tr_z(tray_h-pcb_h)
        rcube(
            [case_wx-2*wall_side+2*pcb_side_margin, case_wy-2*wall_side+2*pcb_side_margin, pcb_h+e],
            r = pcb_r+pcb_side_margin
        );

    }
}


module tray() {
    difference() {
        union() {
            _tray_base();

            magnet_support_4x(tray_inner_h);

            // Inserts holders
            dupe_xy()
            tr_y(insert_pcb_y_offset)
            tr_x(-pcb_wx/2 + insert_pcb_x_offset) tray_pcb_holder();
            
            // PCB supports
            //dupe_xy()
            //tr_xy(-pcb_wx/2, -pcb_wy/2+15) tr_z(wall_hor--e) cube([1.5, 1.5, tray_inner_h]);

            dupe_xy()
            tr_xy(7, -pcb_wy/2) tr_z(wall_hor--e) cube([1.5, 1.5, tray_inner_h]);

            // Button guides
            dupe_x()
            tr_z(wall_hor-e)
            tr_x(btn_pusher_w/2 + 0.1)
            tr_y(-pcb_wy/2 + btn_pusher_pcb_depth + 1)
            mirror_y() cube([2, 4, 1.2]);
        }

        // Button hole
        case_front(wall_side+pcb_support_w+e) tr_z(tray_h/2)
        rotate_x(90)
        linear_extrude(wall_side+pcb_support_w+2e)
        hull() {
            tr_x(btn_w/2-btn_h/2) circle(btn_h/2 + btn_margin);
            tr_x(-btn_w/2+btn_h/2) circle(btn_h/2 + btn_margin);
        }

        // USB connector
        tr_z(tray_h-pcb_h) tr_y(pcb_wy/2+pcb_side_margin) usb_hole();
        
        // Gaskets reserve
        //dupe_xy()
        //tr_xy(-pcb_wx/2 + insert_pcb_x_offset, -15)
        //tr_z(tray_inner_h + wall_hor + e)
        //mirror_z() cylinder(h=1, d=4.8);
    }
}


module cap() {
    union() {
        difference() {
            rcube([case_wx, case_wy, cap_h],
                r = case_r_vert, rbottom = case_r_bottom);

            tr_z(wall_hor)
            rcube(
                [
                    pcb_wx - 2*pcb_support_w,
                    pcb_wy - 2*pcb_support_w,
                    cap_h-wall_hor+e
                ],
                //r = case_r_vert - wall_side
                r = magnet_d/2
            );

            // Inner for pcb alu cover
            wl = wall_side - pcb_side_margin;
            tr_z(cap_h - 2)
            rcube([case_wx-2*wl, case_wy-2*wl, cap_h], r = case_r_vert-wl);
            
            // Small cone conductors for alu cover

            gap = 0.4;

            tr_z(cap_h)
            hull () {
                cube([pcb_wx + gap*2, pcb_wy-2*(magnet_d/2+pcb_support_w), e], center=true);
                tr_z(-gap) cube([pcb_wx, pcb_wy-2*(magnet_d/2+pcb_support_w), e], center=true);

            };

            tr_z(cap_h)
            hull () {
                cube([pcb_wx-2*(magnet_d/2+pcb_support_w), pcb_wy + gap*2, e], center=true);
                tr_z(-gap) cube([pcb_wx-2*(magnet_d/2+pcb_support_w), pcb_wy, e], center=true);

            };
        }

        magnet_support_4x(cap_inner_h);
    }
}

module button() {
    btn_middle_h = tray_h/2 - wall_hor;
    btn_inner_depth = btn_pusher_pcb_depth - pcb_support_w;
    // Calculate back size to leave 0.2mm to led
    btn_back_h = (tray_inner_h - led_h - btn_middle_h - 0.2)*2;

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
            linear_extrude(wall_side + pcb_support_w + 1)
            rsquare([btn_w, btn_h], r=1.5);

            // Inner
            linear_extrude(btn_inner_depth)
            rsquare([btn_pusher_w, btn_back_h], r=0.5);

            // Pusher
            tr_xy(-btn_pusher_w/2, btn_middle_h)
            mirror_y()
            linear_extrude(btn_pusher_base_w)
            rsquare([btn_pusher_w, tray_inner_h-0.2], r=0.5, center=false);

            // extra supports to keep button in the middle of tray hole
            tr_x(-btn_pusher_w/2)
            linear_extrude(btn_inner_depth-0.5) square([1, btn_middle_h]);

            mirror_x()
            tr_x(-btn_pusher_w/2)
            linear_extrude(btn_inner_depth-0.5) square([1, btn_middle_h]);
        }

        // Light mirror
        mside = btn_back_h - 1;
        m_w = btn_pusher_w - 2;

        tr_z(btn_pusher_base_w)
        tr_xy(m_w/2, btn_back_h/2+e) rotate_y(-90)
        linear_extrude(m_w)
        polygon([[0, 0], [mside, 0], [0, -mside]]);
    }
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