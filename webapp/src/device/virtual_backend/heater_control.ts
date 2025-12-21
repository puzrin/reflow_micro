import { Head } from './head'
import { Power } from './power'
import { ADRC } from './adrc'
import { SparseHistory } from '@/device/sparse_history'
import { HeadParams, HistoryChunk, Constants, DeviceActivityStatus } from '@/proto/generated/types'
import type { HEATER_DATA } from './heater_data'
import type { POWER_DATA } from './power_data'
import type { HeaterTask } from './tasks/heater_task'

export class HeaterControl {
  head: Head
  power: Power
  adrc = new ADRC()

  // Temperature control state
  temperature_control_enabled = false
  temperature_setpoint = 0

  // History management (private, managed internally like in firmware)
  private history = new SparseHistory()
  private task_start_ts = 0
  private history_version = 0
  private history_task_id = 0
  private history_last_recorded_ts = 0 // in seconds

  // Task management
  private current_task: HeaterTask | null = null
  private task_iterator: Generator<void, void, unknown> | null = null
  is_task_active = false

  // Callback to access head params (similar to firmware's delegation to Head)
  private get_head_params_fn: (() => HeadParams) | null = null
  private set_head_params_fn: ((params: HeadParams) => void) | null = null

  // Ticker for simulation
  private TICK_PERIOD_MS = 50
  private TIME_ACCELERATION = 10
  private ticker_id: ReturnType<typeof setInterval> | null = null
  private prev_tick_ms = 0

  constructor() {
    this.head = new Head()
    this.power = new Power(this.head)

    // Initialize prev_tick_ms
    this.prev_tick_ms = this.get_time_ms() - this.TICK_PERIOD_MS

    // Start ticker automatically - increase simulation speed 10x
    this.ticker_id = setInterval(() => this.tick(), this.TICK_PERIOD_MS / this.TIME_ACCELERATION)
  }

  get_time_ms(): number {
    return Date.now() * this.TIME_ACCELERATION
  }

  set_params_callbacks(
    get_head_params_fn: () => HeadParams,
    set_head_params_fn: (params: HeadParams) => void
  ): void {
    this.get_head_params_fn = get_head_params_fn
    this.set_head_params_fn = set_head_params_fn
  }

  setup(heater_data: HEATER_DATA, power_data: POWER_DATA[]): this {
    this.head.setup(heater_data)
    this.power.setup(power_data)
    return this
  }

  tick(): void {
    const now = this.get_time_ms()
    const dt_ms = now - this.prev_tick_ms

    // 1. PHYSICS: Apply power from previous tick, simulate temperature change
    const max_power_mw = this.power.get_max_power_mw()
    const actual_power_mw = Math.min(this.power.selector.target_power_mw, max_power_mw)
    this.head.iterate(actual_power_mw / 1000, dt_ms / 1000)

    // 2. POWER: Update ProfileSelector with new resistance (may switch PDO)
    this.power.tick()

    // 3. ADRC: Calculate power for next tick (only if task active)
    if (this.is_task_active && this.temperature_control_enabled) {
      const max_power_w = this.power.get_max_power_mw() / 1000
      const power_w = this.adrc.iterate(
        this.head.temperature,
        this.temperature_setpoint,
        max_power_w,
        dt_ms / 1000
      )
      this.power.set_power_mw(power_w * 1000)
    }

    // 4. Write history every second (like firmware does)
    if (this.is_task_active) {
      const task_time_ms = now - this.task_start_ts
      const seconds = Math.floor(task_time_ms / 1000)

      if (seconds > this.history_last_recorded_ts) {
        this.history.add({
          x: seconds,
          y: this.get_temperature()
        })
        this.history_last_recorded_ts = seconds
      }
    }

    // 5. Run task iterator if active
    if (this.is_task_active && this.task_iterator) {
      const task_time_ms = now - this.task_start_ts
      const result = this.task_iterator.next(task_time_ms)
      if (result.done) {
        this.task_stop()
      }
    }

    this.prev_tick_ms = now
  }

  task_start(task: HeaterTask): boolean {
    if (this.is_task_active) {
      return false
    }

    // Call task.start() for preparation/validation
    if (!task.start()) {
      return false
    }

    this.current_task = task

    // Initialize history (like firmware does)
    this.history.reset()
    this.task_start_ts = this.get_time_ms()
    this.history_last_recorded_ts = 0
    this.history_task_id = task.historyId
    this.history_version++

    // Add first point
    this.history.add({
      x: 0,
      y: this.get_temperature()
    })

    this.task_iterator = task.iterator
    this.is_task_active = true

    // Initialize task iterator with time=0
    this.task_iterator.next(0)

    return true
  }

  task_stop(): void {
    this.is_task_active = false
    this.task_iterator = null
    this.current_task = null
    this.temperature_control_off()
  }

  get_activity_status(): DeviceActivityStatus {
    return this.current_task?.activityId ?? DeviceActivityStatus.Idle
  }

  set_power(watts: number): void {
    this.power.set_power_mw(watts * 1000)
  }

  get_power(): number {
    return this.power.selector.target_power_mw / 1000
  }

  set_temperature(temp: number): void {
    this.temperature_setpoint = temp
  }

  get_temperature(): number {
    return this.head.temperature
  }

  get_max_power(): number {
    return this.power.get_max_power_mw() / 1000
  }

  get_peak_mv(): number {
    return this.power.peak_mv
  }

  get_peak_ma(): number {
    return this.power.peak_ma
  }

  get_duty_x1000(): number {
    return this.power.duty_x1000
  }

  get_resistance_mohms(): number {
    return this.power.get_load_mohm()
  }

  temperature_control_on(head_params: HeadParams): void {
    this.adrc.set_params(
      head_params.adrc_b0,
      head_params.adrc_response,
      head_params.adrc_N,
      head_params.adrc_M
    )
    this.adrc.reset_to(this.head.temperature)
    this.temperature_control_enabled = true
  }

  temperature_control_off(): void {
    this.temperature_control_enabled = false
    this.set_power(0)
    this.set_temperature(this.head.get_room_temp())
  }

  get_head_params(): HeadParams {
    if (!this.get_head_params_fn) {
      throw new Error('Head params callbacks not set')
    }
    return this.get_head_params_fn()
  }

  set_head_params(params: HeadParams): void {
    if (!this.set_head_params_fn) {
      throw new Error('Head params callbacks not set')
    }
    this.set_head_params_fn(params)
  }

  get_history(client_history_version: number, from: number): HistoryChunk {
    const data = this.history.data
    let from_idx = 0
    let chunk_length = 0

    if (this.history_version !== client_history_version) {
      // If client version mismatch - send from the beginning
      from_idx = 0
      chunk_length = Math.min(data.length, Constants.MAX_HISTORY_CHUNK)
    } else {
      if (data.length === 0 || data[data.length - 1].x < from) {
        // If no data - send empty
        from_idx = 0
        chunk_length = 0
      } else {
        // Search from the back, that's usually faster
        if (data[0].x >= from) {
          // Special case, nothing to skip
          from_idx = 0
        } else {
          for (let i = data.length - 1; i >= 0; i--) {
            if (data[i].x < from) {
              from_idx = i + 1
              break
            }
          }
        }
        chunk_length = Math.min(data.length - from_idx, Constants.MAX_HISTORY_CHUNK)
      }
    }

    return {
      type: this.history_task_id,
      version: this.history_version,
      data: data.slice(from_idx, from_idx + chunk_length)
    }
  }
}
