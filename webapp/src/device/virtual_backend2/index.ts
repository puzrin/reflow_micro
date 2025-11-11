import { HeaterControl } from './heater_control'
import { useVirtualBackendStore } from './virtualBackendStore'
import { Device, type IBackend } from '@/device'
import { TaskSensorBake } from './tasks/task_sensor_bake'
import { TaskAdrcTest } from './tasks/task_adrc_test'
import { TaskReflow } from './tasks/task_reflow'
import { TaskStepResponse } from './tasks/task_step_response'
import { mcpcb_80x70x16 as heater_data } from './heater_data'
import { charger_140w_pps as power_data } from './power_data'
import {
  ProfilesData,
  HeadParams,
  HistoryChunk,
  DeviceHealthStatus,
  DeviceActivityStatus,
  HeadStatus,
  Constants,
  PowerStatus
} from '@/proto/generated/types'
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/defaults'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
  static id: string = 'virtual' as const

  private device: Device
  private heater_control: HeaterControl

  // Used to simulate remote history sync
  client_history_version: number = -1

  constructor(device: Device) {
    this.device = device

    // Setup heater control
    this.heater_control = new HeaterControl().setup(heater_data, power_data)

    // Set up callbacks for head params access (similar to firmware's delegation)
    this.heater_control.set_params_callbacks(
      () => this.pick_head_params(),
      (params) => this.set_head_params(params)
    )
  }

  async fetch_status(): Promise<void> {
    if (!this.device.is_ready.value) return

    Object.assign(this.device.status, {
      health: DeviceHealthStatus.DevOK,
      activity: this.heater_control.get_activity_status(),
      power: PowerStatus.PwrOK,
      head: HeadStatus.HeadConnected,
      temperature_x10: Math.round(this.heater_control.get_temperature() * 10),
      peak_mv: Math.round(this.heater_control.get_peak_mv()),
      peak_ma: Math.round(this.heater_control.get_peak_ma()),
      duty_x1000: this.heater_control.get_duty_x1000(),
      resistance_mohms: Math.round(this.heater_control.get_resistance_mohms()),
      max_mw: Math.round(this.heater_control.get_max_power() * 1000)
    })
  }

  async fetch_history(): Promise<void> {
    if (!this.device.is_ready.value) return

    const len = this.device.history.points.length
    const from = len ? this.device.history.points[len - 1].x : 0

    const history_slice = await this.get_history_slice(this.client_history_version, from)

    if (history_slice.version === this.client_history_version) {
      // Merge update
      this.device.sparseHistory.add(...history_slice.data)
    } else {
      // Full replace
      this.client_history_version = history_slice.version
      this.device.history.points.splice(0, this.device.history.points.length, ...history_slice.data)
      this.device.history.id = history_slice.type
    }

    // If data size is max allowed => it could be shrinked => repeat request
    if (history_slice.data.length >= Constants.MAX_HISTORY_CHUNK) {
      await this.fetch_history()
    }
  }

  async attach() {
    this.device.is_connecting.value = true
    this.device.is_connected.value = true
    this.device.need_pairing.value = false
    this.device.is_authenticated.value = true
    this.client_history_version = -1

    this.device.status.head = HeadStatus.HeadConnected

    await this.device.loadProfilesData()

    this.device.is_ready.value = true
  }

  async detach() {
    if (this.heater_control.is_task_active) await this.stop()
    this.device.is_connecting.value = false
    this.device.is_connected.value = false
    this.device.is_authenticated.value = false
    this.device.is_ready.value = false

    this.device.status.head = HeadStatus.HeadDisconnected
  }

  async connect() {}

  async load_profiles_data(reset = false): Promise<ProfilesData> {
    const virtualBackendStore = useVirtualBackendStore()

    if (reset) await this.save_profiles_data(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB))

    return structuredClone(ProfilesData.fromJSON(virtualBackendStore.rawProfilesData))
  }

  async save_profiles_data(data: ProfilesData): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawProfilesData = ProfilesData.toJSON(data)
  }

  async stop() {
    this.heater_control.task_stop()
  }

  private async get_history_slice(version: number, from: number): Promise<HistoryChunk> {
    return this.heater_control.get_history(version, from)
  }

  async run_reflow() {
    if (this.heater_control.is_task_active) throw new Error('Cannot start profile, device busy')

    const task = new TaskReflow(this.heater_control)
    if (!this.heater_control.task_start(task)) {
      throw new Error('Failed to start reflow')
    }
  }

  async run_sensor_bake(watts: number) {
    const activity = this.heater_control.get_activity_status()

    if (activity === DeviceActivityStatus.SensorBake) {
      // Task already running, just update power
      this.heater_control.set_power(watts)
      return
    }

    if (this.heater_control.is_task_active) {
      throw new Error('Cannot heat sensor, device busy')
    }

    const task = new TaskSensorBake(this.heater_control, watts)
    if (!this.heater_control.task_start(task)) {
      throw new Error('Failed to start sensor bake')
    }
  }

  async run_adrc_test(temperature: number) {
    const activity = this.heater_control.get_activity_status()

    if (activity === DeviceActivityStatus.AdrcTest) {
      // Task already running, just update temperature
      this.heater_control.set_temperature(temperature)
      return
    }

    if (this.heater_control.is_task_active) {
      throw new Error('Cannot run test, device busy')
    }

    const task = new TaskAdrcTest(this.heater_control, temperature)
    if (!this.heater_control.task_start(task)) {
      throw new Error('Failed to start ADRC test')
    }
  }

  async run_step_response(watts: number) {
    if (this.heater_control.is_task_active) throw new Error('Cannot run test, device busy')

    const task = new TaskStepResponse(this.heater_control, watts)
    if (!this.heater_control.task_start(task)) {
      throw new Error('Failed to start step response')
    }
  }

  // Sync, for local use from tasks
  pick_head_params(): HeadParams {
    const virtualBackendStore = useVirtualBackendStore()
    return structuredClone(HeadParams.fromJSON(virtualBackendStore.rawHeadParams))
  }

  async get_head_params(): Promise<HeadParams> {
    return this.pick_head_params()
  }

  async set_head_params(config: HeadParams): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawHeadParams = HeadParams.toJSON(config)
  }

  async set_cpoint0(temperature: number): Promise<void> {
    const params = this.pick_head_params()
    // Note, we do not implement temperature correction in simulator,
    // just store values for proper display in UI
    params.sensor_p0_at = this.heater_control.get_temperature()
    params.sensor_p0_value = temperature
    await this.set_head_params(params)
  }

  async set_cpoint1(temperature: number): Promise<void> {
    const params = this.pick_head_params()
    params.sensor_p1_at = this.heater_control.get_temperature()
    params.sensor_p1_value = temperature
    await this.set_head_params(params)
  }

  async get_ble_name(): Promise<string> {
    const { useLocalSettingsStore } = await import('@/stores/localSettings')
    const localSettingsStore = useLocalSettingsStore()
    return localSettingsStore.bleName
  }

  async set_ble_name(name: string): Promise<void> {
    const { useLocalSettingsStore } = await import('@/stores/localSettings')
    const localSettingsStore = useLocalSettingsStore()
    localSettingsStore.bleName = name
  }
}
