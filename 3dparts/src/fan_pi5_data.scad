// Raspberry Pi5 fan data.
//
// - Top view
// - Air flow to the left
// - Wire at the right bottom corner
// - Size: 30x30 mm

// d = 1.95 mm in plastic, 2.05 mm in metal
fan_mount_coords = [
    [ 4.275, 28.245, 0],  // x =  3.25 + 1.025, y = 27.22 + 1.025
    [27.255, 27.325, 0],  // x = 26.23 + 1.025, y = 26.30 + 1.025
    [27.255,  2.455, 0]   // x = 26.23 + 1.025, y = 1.43  + 1.025 
];

// d = 21.2 mm
// y = 5.1 + d/2
// x = 3.6 + d/2
fan_hole_coords = [14.2, 15.7, 0];