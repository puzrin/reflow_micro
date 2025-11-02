export type POWER_DATA = {
  mv_min: number
  mv_max: number
  ma_max: number
}

// Simulated charger: 140W with PPS support
export const charger_140w_pps: POWER_DATA[] = [
  { mv_min: 9000, mv_max: 9000, ma_max: 3000 },    // FIXED 9V/3A
  { mv_min: 12000, mv_max: 12000, ma_max: 3000 },  // FIXED 12V/3A
  { mv_min: 15000, mv_max: 15000, ma_max: 3000 },  // FIXED 15V/3A
  { mv_min: 20000, mv_max: 20000, ma_max: 5000 },  // FIXED 20V/5A
  { mv_min: 5000, mv_max: 21000, ma_max: 5000 },   // PPS 5-21V/5A
  { mv_min: 28000, mv_max: 28000, ma_max: 5000 },  // FIXED 28V/5A
]
