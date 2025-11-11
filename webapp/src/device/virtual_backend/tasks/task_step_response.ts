import type { HeaterControl } from '../heater_control'
import { HeaterTask } from './heater_task'
import { DeviceActivityStatus, Constants } from '@/proto/generated/types'

const LOG_INTERVAL_MS = 500
const MAX_TRANSPORT_DELAY_MS = 10_000
const STABILITY_CHECK_PERIOD_MS = 30_000
const STABILITY_THRESHOLD_TEMP = 1.0
const MAX_LOG_SIZE = 2000

interface LogEntry {
  temperature: number
  power: number
  time_ms: number
}

export class TaskStepResponse extends HeaterTask {
  historyId = Constants.HISTORY_ID_STEP_RESPONSE
  readonly activityId = DeviceActivityStatus.StepResponse

  constructor(
    private heater: HeaterControl,
    private watts: number
  ) {
    super()
  }

  start(): boolean {
    this.heater.set_power(this.watts)
    return true
  }

  get iterator(): Generator<void, void, number> {
    const heater = this.heater
    return (function* () {
      let next_record_ts = 0
      const temperature_log: LogEntry[] = []

      while (true) {
        const task_time_ms: number = yield
        const probe = heater.get_temperature()

        if (task_time_ms >= next_record_ts) {
          next_record_ts += LOG_INTERVAL_MS

          // Check for abnormal temperature jitter
          if (temperature_log.length > 0) {
            const last_temp = temperature_log[temperature_log.length - 1].temperature
            if (Math.abs(probe - last_temp) > 5.0) {
              console.error(`Abnormal temperature jitter detected: ${last_temp.toFixed(1)} -> ${probe.toFixed(1)}`)
            }
          }

          // Record new log entry
          temperature_log.push({
            temperature: probe,
            power: heater.get_power(),
            time_ms: task_time_ms
          })

          if (temperature_log.length >= MAX_LOG_SIZE) {
            console.log('Max log size reached')
            return
          }

          // Skip transport delay period
          if (task_time_ms < MAX_TRANSPORT_DELAY_MS) {
            continue
          }

          // Check stability
          const offset_entries = STABILITY_CHECK_PERIOD_MS / LOG_INTERVAL_MS
          if (temperature_log.length > offset_entries) {
            const old_entry = temperature_log[temperature_log.length - offset_entries - 1]
            const current_entry = temperature_log[temperature_log.length - 1]

            if (Math.abs(current_entry.temperature - old_entry.temperature) <= STABILITY_THRESHOLD_TEMP) {
              break
            }
          }
        }
      }

      // Analyze log to find response time & b0
      const t_initial = temperature_log[0].temperature

      // Find maximum temperature in log
      let t_max = temperature_log[0].temperature
      for (let i = 1; i < temperature_log.length; i++) {
        if (temperature_log[i].temperature > t_max) {
          t_max = temperature_log[i].temperature
        }
      }

      // Helper function to find index at ratio of (t_max - t_initial)
      const find_t_idx_of = (ratio: number): number => {
        if (ratio >= 1.0) {
          let max_idx = 0
          for (let i = 1; i < temperature_log.length; i++) {
            if (temperature_log[i].temperature > temperature_log[max_idx].temperature) {
              max_idx = i
            }
          }
          return max_idx
        }

        const target_temp = t_initial + (t_max - t_initial) * ratio

        let idx = temperature_log.length - 1
        for (let i = 0; i < temperature_log.length; i++) {
          if (temperature_log[i].temperature >= target_temp) {
            idx = i
            break
          }
        }

        if (idx > 0) {
          const diff_curr = Math.abs(temperature_log[idx].temperature - target_temp)
          const diff_prev = Math.abs(temperature_log[idx - 1].temperature - target_temp)

          if (diff_prev < diff_curr) {
            idx = idx - 1
          }
        }

        return idx
      }

      const idx_28 = find_t_idx_of(0.28)
      const idx_63 = find_t_idx_of(0.63)

      const c_28 = temperature_log[idx_28].temperature
      const t_28 = temperature_log[idx_28].time_ms / 1000
      const c_63 = temperature_log[idx_63].temperature
      const t_63 = temperature_log[idx_63].time_ms / 1000

      const τ = 1.502069 * (t_63 - t_28)
      const L = 1.493523 * t_28 - 0.493523 * t_63

      console.log('Step response analysis:')
      console.log(`  t max = ${t_max.toFixed(0)}°C`)
      console.log(`  P1(28%) = ${t_28.toFixed(0)}°C, ${c_28.toFixed(0)}sec`);
      console.log(`  P2(63%) = ${t_63.toFixed(0)}°C, ${c_63.toFixed(0)}sec`);
      console.log(`  response = ${τ.toFixed(2)}s, effective delay = ${L.toFixed(2)}s`)

      const du = temperature_log[idx_63].power
      const b0 = (c_63 - c_28) / ((0.63 - 0.28) * τ * du)
      console.log(`  b0 = ${b0.toFixed(6)}`)

      // Update head params
      const head_params = heater.get_head_params()
      head_params.adrc_response = τ
      head_params.adrc_b0 = b0
      heater.set_head_params(head_params)
    })()
  }
}
