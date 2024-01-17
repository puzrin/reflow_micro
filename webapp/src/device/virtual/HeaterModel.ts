import { ref } from "vue"
import { type IHeaterModel, TICK_PERIOD_MS } from "../types"
import { clamp } from "../utils"

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

export class HeaterModel implements IHeaterModel {
  private _setPointWatts: number = 0
  private Q: number = C * Ta

  temperature = ref(Ta)
  resistance = ref(ambientHeaterResistance)
  volts = ref(0)
  amperes = ref(0)
  watts = ref(0)

  // PD profile limits
  maxVolts = ref(20)
  maxAmperes = ref(3.25)

  // Depends on resistance at current temperature and PD profile limits
  maxWatts = ref(65)

  setPoint(watts: number) { this._setPointWatts = watts }

  tick() {
    // First, restrict applied power to really possible
    const R = heaterResistance(this.temperature.value)
    const maxW = Math.min(this.maxVolts.value ** 2 / R, this.maxAmperes.value ** 2 * R)
    const clampedW = clamp(this._setPointWatts, 0, maxW)

    const dt = TICK_PERIOD_MS / 1000
    const T = this.Q/C
    this.Q += clampedW * dt
    this.Q -= K*(T-Ta)*dt

    this.resistance.value = R
    this.maxWatts.value = maxW
    this.temperature.value = T
    this.watts.value = clampedW
    this.volts.value = Math.sqrt(clampedW * R)
    this.amperes.value = Math.sqrt(clampedW / R)
  }
}
