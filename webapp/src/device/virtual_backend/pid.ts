import { clamp } from '../utils'
import { TICK_PERIOD_MS } from './'

export class PID {
  private Kp: number = 0
  private Ki: number = 0
  private Kd: number = 0

  private _prevFeedback: number = 0
  private _target: number = 0
  private I: number = 0

  private outMin: number = 0
  private outMax: number = 0


  tick(feedback: number) {
    const error = this._target - feedback

    const P = this.Kp * error

    // Integral term
    const dt = TICK_PERIOD_MS / 1000
    this.I += this.Ki * error * dt

    // Anti-windup. Clamp I to fit (P+I) in output range
    const maxI = (P < this.outMax) ? this.outMax - P : 0
    const minI = (P > this.outMin) ? this.outMin - P : 0
    this.I = clamp(this.I, minI, maxI)

    // Derivative term
    const D = this.Kd * (feedback - this._prevFeedback) / dt
    this._prevFeedback = feedback

    // Clamp again to fit (P+I+D) in output range
    const output = clamp(P + this.I + D, this.outMin, this.outMax)

    return output
  }

  setK(Kp: number, Ki: number, Kd: number) {
    this.Kp = Kp
    this.Ki = Ki
    this.Kd = Kd
    return this
  }

  setLimits(min: number, max: number) {
    this.outMin = min
    this.outMax = max
    return this
  }

  setPoint(target: number) {
    this._target = target
    return this
  }

  reset() {
    this._prevFeedback = 0
    this._target = 0
    this.I = 0
    return this
  }
}