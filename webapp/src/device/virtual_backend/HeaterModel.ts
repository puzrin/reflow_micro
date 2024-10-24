import { ref } from "vue"
import { clamp } from "./utils"

// ~ 300° @ 60W

const aluminumSHC = 881 // Specific heat capacity, J/kg/K
const aluminumDensity = 2700 // kg/m3

const plateSize = { x: 0.07, y: 0.06, z: 0.001 } // 70x60x3mm
const plateVolume = plateSize.x * plateSize.y * plateSize.z // 12.6 cm³
const plateSquare = plateSize.x * plateSize.y // 42 cm²
const plateWeight = plateVolume * aluminumDensity // 34g

// Ambient temperature [C]
const Ta = 20

// Heat capacity [J/K]
const C = aluminumSHC * plateWeight

// Heat transfer coefficient [W/K]
// "Magical" constant to reach temperature of real device at max power
const K = 50 * plateSquare

// Tungsten temperature coefficient [°C/W] at 20°C
const tungstenTC = 0.0041

const ambientHeaterResistance = 3.3 // Ohm

function heaterResistance(temperature: number) {
  return ambientHeaterResistance * (1 + tungstenTC * (temperature - 20))
}

export class HeaterModel {
  private _setPointWatts: number = 0
  private Q: number = C * Ta

  temperature = Ta
  resistance = ambientHeaterResistance
  volts = 0
  amperes = 0
  watts = 0

  // PD profile limits
  maxVolts = 20
  maxAmperes = 5

  // Depends on resistance at current temperature and PD profile limits
  maxWatts = 100

  setPoint(watts: number) { this._setPointWatts = watts }

  tick(ms_period: number) {
    // First, restrict applied power to really possible
    const R = heaterResistance(this.temperature)
    const maxW = Math.min(this.maxVolts ** 2 / R, this.maxAmperes ** 2 * R)
    const clampedW = clamp(this._setPointWatts, 0, maxW)

    const dt = ms_period / 1000
    const T = this.Q/C
    this.Q += clampedW * dt
    this.Q -= K*(T-Ta)*dt

    this.resistance = R
    this.maxWatts = maxW
    this.temperature = T
    this.watts = clampedW
    this.volts = Math.sqrt(clampedW * R)
    this.amperes = Math.sqrt(clampedW / R)
  }
}
