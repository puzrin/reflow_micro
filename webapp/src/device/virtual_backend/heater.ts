import { ADRC } from './adrc'

function interpolate(x: number, points: [number, number][]): number {
    if (points.length < 2) {
        throw new Error("At least two points are required for interpolation.")
    }

    if (x <= points[0][0]) {
        const [x1, y1] = points[0]
        const [x2, y2] = points[1]
        return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
    }

    if (x >= points[points.length - 1][0]) {
        const [x1, y1] = points[points.length - 2]
        const [x2, y2] = points[points.length - 1]
        return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
    }

    for (let i = 1; i < points.length; i++) {
        if (x < points[i][0]) {
            const [x1, y1] = points[i - 1]
            const [x2, y2] = points[i]
            return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1)
        }
    }

    throw new Error("Interpolation error: x is out of bounds.")
}

export class PDProfile {
    V: number;
    I: number;
    PPS: boolean;

    constructor(V: number, I: number, PPS = false) {
        this.V = V;
        this.I = I;
        this.PPS = PPS;
    }

    is_useable(R: number): boolean {
        return this.PPS ? true : (this.V / R) <= this.I;
    }

    get_power(R: number): number {
        if (this.PPS) {
            const V_max = Math.min(this.V, this.I * R);
            return (V_max ** 2) / R;
        }
        return this.is_useable(R) ? (this.V ** 2) / R : 0;
    }
}

export class ChargerProfiles {
    profiles: PDProfile[];

    constructor() {
        this.profiles = [];
    }

    add(profile: PDProfile): this {
        this.profiles.push(profile);
        return this;
    }

    get_best_profile(R: number): PDProfile | null {
        const usableProfiles = this.profiles.filter(p => p.is_useable(R));
        if (usableProfiles.length === 0) return null;
        return usableProfiles.reduce((best, current) =>
            current.get_power(R) > best.get_power(R) ? current : best
        );
    }

    getPower(R: number): number {
        const bestProfile = this.get_best_profile(R);
        return bestProfile ? bestProfile.get_power(R) : 0;
    }
}

// Define several charger configurations

export const charger_140w_fixed_only = new ChargerProfiles()
    .add(new PDProfile(9, 3))
    .add(new PDProfile(12, 3))
    .add(new PDProfile(15, 3))
    .add(new PDProfile(20, 5))
    .add(new PDProfile(28, 5));

export const charger_140w_with_pps = new ChargerProfiles()
    .add(new PDProfile(9, 3))
    .add(new PDProfile(12, 3))
    .add(new PDProfile(15, 3))
    .add(new PDProfile(20, 5))
    .add(new PDProfile(21, 5, true))
    .add(new PDProfile(28, 5));


export class Heater {
    static TUNGSTEN_TC = 0.0041 // Temperature coefficient for tungsten

    size: { x: number; y: number; z: number }
    calibration_points: { T: number; R: number; W: number }[]
    temperature: number
    profiles: ChargerProfiles

    power_setpoint: number;
    temperature_setpoint: number;

    adrc: ADRC = new ADRC()
    private temperature_control_enabled: boolean = false

    constructor(x = 0.08, y = 0.07, z = 0.0038) {
        this.size = { x, y, z } // Plate dimensions in meters
        this.calibration_points = [] // Calibration points for resistance and heat dissipation
        this.temperature = this.get_room_temp() // Current temperature
        this.profiles = charger_140w_with_pps
        this.power_setpoint = 0
        this.temperature_setpoint = this.temperature
    }

    clone(): Heater {
        // Create a new instance with the same properties
        const new_instance = new Heater(
            this.size.x,
            this.size.y,
            this.size.z
        )
        new_instance.calibration_points = this.calibration_points.map((point) => ({ ...point }))
        new_instance.temperature = this.temperature
        new_instance.power_setpoint = this.power_setpoint
        new_instance.profiles = this.profiles
        return new_instance
    }

    reset(): this {
        // Reset temperature to the initial calibration point or room temperature
        this.temperature = this.get_room_temp()
        return this
    }

    set_profiles(profiles: ChargerProfiles): this {
        this.profiles = profiles
        return this
    }

    set_size(x: number, y: number, z: number) { this.size = { x, y, z }; return this }

    scale_r_to(new_base: number): this {
        // Scale the resistance in all calibration points, to simplify configuration for different heaters
        const ratio = new_base / this.calibration_points[0].R
        for (const point of this.calibration_points) {
            point.R *= ratio
        }
        return this
    }

    calibrate(options: { T: number, R?: number; W?: number; V?: number; I?: number }): this {
        const { T, R, W, V, I } = options
        if (R !== undefined && W === undefined && V === undefined && I === undefined) {
            // Case: {T, R}
            this.calibration_points.push({ T, R, W: 0 })
            this.temperature = T // Set initial temperature based on the room temperature calibration point
        } else if (W !== undefined && R !== undefined && V === undefined && I === undefined) {
            // Case: {T, R, W}
            this.calibration_points.push({ T, R, W })
        } else if (W !== undefined && V !== undefined && R === undefined && I === undefined) {
            // Case: {T, W, V}
            this.calibration_points.push({ T, R: (V ** 2) / W, W })
        } else if (W !== undefined && I !== undefined && R === undefined && V === undefined) {
            // Case: {T, W, I}
            this.calibration_points.push({ T, R: W / (I ** 2), W })
        } else {
            throw new Error("Invalid combination of calibration inputs. Must provide either {T, R} or {T, W, R | V | I}.")
        }

        // Sort calibration points by temperature and check that resistance is non-decreasing
        this.calibration_points.sort((a, b) => a.T - b.T)

        // Minimal validation
        for (let i = 1; i < this.calibration_points.length; i++) {
            if (this.calibration_points[i].R < this.calibration_points[i - 1].R) {
                throw new Error("Calibration points must have non-decreasing resistance values for increasing temperatures.")
            }
        }

        return this
    }

    temperature_control_on() {
        this.adrc.reset_to(this.temperature)
        this.temperature_control_enabled = true
        return this
    }

    temperature_control_off() {
        this.temperature_control_enabled = false
        this.set_power(0)
        this.set_temperature(this.get_room_temp())
        return this
    }

    iterate(dt: number): void {
        const clamped_power = this.get_power()

        const heat_capacity = this.calculate_heat_capacity()
        const heat_transfer_coefficient = this.calculate_heat_transfer_coefficient()
        const temperature_change = ((clamped_power - heat_transfer_coefficient * (this.temperature - this.get_room_temp())) * dt) / heat_capacity

        this.temperature += temperature_change

        if (this.temperature_control_enabled) {
            const power = this.adrc.iterate(this.temperature, this.temperature_setpoint, this.get_max_power(), dt)
            this.set_power(power)
        }
    }

    calculate_resistance(temperature: number): number {
        if (this.calibration_points.length === 0) throw new Error("No calibration points defined.")

        if (this.calibration_points.length === 1) {
            const single_point = this.calibration_points[0]
            return single_point.R * (1 + Heater.TUNGSTEN_TC * (temperature - single_point.T))
        }

        const points = this.calibration_points.map((p) => [p.T, p.R] as [number, number])
        return interpolate(temperature, points)
    }

    calculate_heat_capacity(): number {
        const material_shc = 897 // J/kg/K for Aluminum 6061
        const material_density = 2700 // kg/m3 for Aluminum 6061
        const volume = this.size.x * this.size.y * this.size.z // in m³
        const mass = volume * material_density // in kg
        return mass * material_shc
    }

    calculate_heat_transfer_coefficient(): number {
        const points_with_power = this.calibration_points.filter((p) => p.W !== 0)

        if (!points_with_power.length) {
            const area = this.size.x * this.size.y // in m²
            return 40 * area // in W/K, Default empiric value, when no calibration data is available
        }

        if (points_with_power.length === 1) {
            const point = points_with_power[0]
            return point.W / (point.T - this.get_room_temp())
        }

        const points = points_with_power.map(
            (p) => [p.T, p.W / (p.T - this.get_room_temp())] as [number, number]
        )
        return interpolate(this.temperature, points)
    }

    set_power(power: number) { this.power_setpoint = power < 0 ? 0 : power; return this }

    set_temperature(temperature: number) { this.temperature_setpoint = temperature; return this }

    get_room_temp() {
        const room_temp_point = this.calibration_points.find((p) => p.W === 0)
        return room_temp_point ? room_temp_point.T : 25 // Default to 25°C if no room temperature point is found
    }

    get_max_power() {
        const R = this.calculate_resistance(this.temperature)
        return this.profiles.getPower(R)
    }

    get_resistance() { return this.calculate_resistance(this.temperature) }

    get_power(){ return Math.min(this.get_max_power(), this.power_setpoint) }

    get_volts() { return Math.sqrt(this.get_power() * this.get_resistance()) }

    get_amperes() {
        const r = this.get_resistance()
        return r > 0 ? Math.sqrt(this.get_power() / r) : 0;
    }
}

export const configured_heater: Heater[] = [
    new Heater()
        .calibrate({ T: 25, R: 1.6 })
        .calibrate({ T: 102, W: 11.63, V: 5 })
        .calibrate({ T: 146, W: 20.17, V: 7 }) // 151?
        .calibrate({ T: 193, W: 29.85, V: 9 })
        .calibrate({ T: 220, W: 40.66, V: 11 })
        .calibrate({ T: 255, W: 52.06, V: 13 })
        .calibrate({ T: 286, W: 64.22, V: 15 })
        .calibrate({ T: 310, W: 77.55, V: 17 })
]