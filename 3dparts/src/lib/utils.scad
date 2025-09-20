include <Round-Anything/polyround.scad>

e = 0.01;

function  polySquare(size = [30, 10], r = [1, 1, 1, 1], center = true) =
    let (
        _r0 = (is_num(r) ? r : r[0]),
        _r1 = (is_num(r) ? r : r[1]),
        _r2 = (is_num(r) ? r : r[2]),
        _r3 = (is_num(r) ? r : r[3]),
        r0 = _r0 ? _r0 : e,
        r1 = _r1 ? _r1 : e,
        r2 = _r2 ? _r2 : e,
        r3 = _r3 ? _r3 : e,

        x = size[0], y = size[1],

        tx = center ? -x/2 : 0,
        ty = center ? -y/2 : 0
    )
    [
        [tx + 0, ty + 0, _r0],
        [tx + 0, ty + y, _r1],
        [tx + x, ty + y, _r2],
        [tx + x, ty + 0, _r3]
    ];

module ra_cube(size = [10, 10, 10], r = 1, rtop = 0, rbottom = 0, center = true, fn=16) {
    polyRoundExtrude(
        polySquare([size[0], size[1]], r, center),
        size[2], rtop, rbottom, fn=fn
    );
}

module rotate_x(angle) { rotate([angle, 0, 0]) children(); }
module rotate_y(angle) { rotate([0, angle, 0]) children(); }
module rotate_z(angle) { rotate([0, 0, angle]) children(); }

module tr_x(ofs) { translate([ofs, 0, 0]) children(); }
module tr_y(ofs) { translate([0, ofs, 0]) children(); }
module tr_z(ofs) { translate([0, 0, ofs]) children(); }
module tr_xy(ofs_x, ofs_y) { translate([ofs_x, ofs_y, 0]) children(); }

module mirror_x() { mirror([1, 0, 0]) children(); }
module mirror_y() { mirror([0, 1, 0]) children(); }
module mirror_z() { mirror([0, 0, 1]) children(); }
module mirror_xy() { mirror_x() mirror_y() children(); }

module dupe_x() { children(); mirror_x() children(); }
module dupe_y() { children(); mirror_y() children(); }
module dupe_xy() { children(); mirror_x() children(); mirror_y() children(); mirror_xy() children(); }
