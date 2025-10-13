import { Heater, configured_heater } from './heater'
import { useProfilesStore } from '@/stores/profiles'
import { useVirtualBackendStore } from './virtualBackendStore'
import { Device, type IBackend } from '@/device'
import { SparseHistory } from '@/device/sparse_history'
import { task_sensor_bake } from './tasks/task_sensor_bake'
import { task_adrc_test } from './tasks/task_adrc_test'
import { task_reflow } from './tasks/task_reflow'
import { task_step_response } from './tasks/task_step_response'
import { ProfilesData, HeadParams, HistoryChunk, DeviceHealthStatus, DeviceActivityStatus, HeadStatus, Constants, PowerStatus } from '@/proto/generated/types'
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/defaults'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
  static id: string = 'virtual' as const

  private device: Device
  heater: Heater

  private activity = DeviceActivityStatus.Idle

  private ticker_id: ReturnType<typeof setInterval> | null = null

  private static readonly LS_KEY = 'virtual_backend_profiles_data'
  private task_iterator: Generator | null = null

  // Used to simulate remote history sync.
  client_history_version: number = -1
  remote_history_version: number = 0
  remote_history_id: number = 0
  remote_history: SparseHistory = new SparseHistory() // public, to update from tasks

  constructor(device: Device) {
    this.device = device
    this.heater = configured_heater[0]
      .clone()
      .reset()
      .scale_r_to(4.0)
      .set_size(0.08, 0.07, 0.0028)
      //.set_size(0.07, 0.06, 0.0028)
  }

  async fetch_status(): Promise<void> {
    if (!this.device.is_ready.value) return

    this.device.status.value = {
      health: DeviceHealthStatus.DevOK,
      activity: this.activity,
      power: PowerStatus.PwrOK,
      head: HeadStatus.HeadConnected,
      temperature_x10: Math.round(this.heater.temperature * 10),
      peak_mv: Math.round(this.heater.get_volts() * 1000),
      peak_ma: Math.round(this.heater.get_amperes() * 1000),
      duty_x1000: 1000, // Always 100% in simulation
      resistance_mohms: Math.round(this.heater.get_resistance() * 1000),
      max_mw: Math.round(this.heater.get_max_power() * 1000)
    }
  }

  async fetch_history(): Promise<void> {
    if (!this.device.is_ready.value) return

    const len = this.device.history.value.length
    const from = len ? this.device.history.value[len-1].x : 0

    const history_slice = await this.get_history_slice(this.client_history_version, from)

    if (history_slice.version === this.client_history_version) {
      // Merge update
      this.device.sparseHistory.add(...history_slice.data)
    } else {
      // Full replace
      this.client_history_version = history_slice.version
      this.device.history.value.splice(0, this.device.history.value.length, ...history_slice.data)
      this.device.history_id.value = history_slice.type
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

    this.device.status.value.head = HeadStatus.HeadConnected

    await this.device.loadProfilesData()

    this.device.is_ready.value = true

    // Increase simulation speed 10x
    this.ticker_id = setInterval(() => this._tick(), TICK_PERIOD_MS / 10)
  }

  async detach() {
    if (this.ticker_id !== null) clearInterval(this.ticker_id)
    if (this.activity !== DeviceActivityStatus.Idle) await this.stop()
    this.remote_history.reset()
    this.device.is_connecting.value = false
    this.device.is_connected.value = false
    this.device.is_authenticated.value = false
    this.device.is_ready.value = false

    this.device.status.value.head = HeadStatus.HeadDisconnected
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

  private _tick() {
    // Heater emulator works non-stop mode
    this.heater.iterate(TICK_PERIOD_MS/1000)

    if (this.task_iterator?.next().done === true) this.task_iterator = null
  }

  async stop() {
    this.activity = DeviceActivityStatus.Idle
    this.task_iterator = null
    this.heat_control_off()
  }

  private reset_remote_history(new_id: number) {
    this.remote_history_id = new_id
    this.remote_history_version++
    this.remote_history.reset()
  }

  private async get_history_slice(version: number, from: number): Promise<HistoryChunk> {
    // History outdated => return full new one
    if (this.remote_history_version != version) {
      return {
        type: this.remote_history_id,
        version: this.remote_history_version,
        data: this.remote_history.data.slice(0, Constants.MAX_HISTORY_CHUNK)
      }
    }

    return {
      type: this.remote_history_id,
      version: this.remote_history_version,
      data: this.remote_history.get_data_from(from).slice(0, Constants.MAX_HISTORY_CHUNK)
    }
  }

  async run_reflow() {
    if (this.activity !== DeviceActivityStatus.Idle) throw new Error('Cannot start profile, device busy')

    const profilesStore = useProfilesStore()
    if (profilesStore.selected === null) throw new Error('No profile set')

    this.reset_remote_history(profilesStore.selected.id)
    this.activity = DeviceActivityStatus.Reflow

    this.task_iterator = task_reflow(this, profilesStore.selected)
    this.task_iterator.next()
  }

  async run_sensor_bake(watts: number) {
    if (this.activity !== DeviceActivityStatus.Idle && this.activity !== DeviceActivityStatus.SensorBake) {
      throw new Error('Cannot heat sensor, device busy')
    }

    if (this.activity === DeviceActivityStatus.Idle) {
      this.reset_remote_history(Constants.HISTORY_ID_SENSOR_BAKE_MODE)
      this.activity = DeviceActivityStatus.SensorBake
      this.task_iterator = task_sensor_bake(this)
    }

    this.heater.set_power(watts)
  }

  async run_adrc_test(temperature: number) {
    if (this.activity !== DeviceActivityStatus.Idle && this.activity !== DeviceActivityStatus.AdrcTest) {
      throw new Error('Cannot run test, device busy')
    }

    if (this.activity === DeviceActivityStatus.Idle) {
      this.reset_remote_history(Constants.HISTORY_ID_ADRC_TEST_MODE)
      this.activity = DeviceActivityStatus.AdrcTest
      this.task_iterator = task_adrc_test(this)
    }

    this.heater.set_temperature(temperature)
  }

  async run_step_response(watts: number) {
    if (this.activity !== DeviceActivityStatus.Idle) throw new Error('Cannot run test, device busy')

    this.reset_remote_history(Constants.HISTORY_ID_STEP_RESPONSE)
    this.activity = DeviceActivityStatus.StepResponse
    this.task_iterator = task_step_response(this, watts)
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
    params.sensor_p0_at = this.heater.temperature
    params.sensor_p0_value = temperature
    await this.set_head_params(params)
  }

  async set_cpoint1(temperature: number): Promise<void> {
    const params = this.pick_head_params()
    params.sensor_p1_at = this.heater.temperature
    params.sensor_p1_value = temperature
    await this.set_head_params(params)
  }

  heat_control_on() {
    const head_params = this.pick_head_params()
    this.heater.adrc.set_params(head_params.adrc_b0, head_params.adrc_response, head_params.adrc_N, head_params.adrc_M)
    this.heater.temperature_control_on()
  }

  heat_control_off() {
    this.heater.temperature_control_off()
    this.heater.set_temperature(this.heater.get_room_temp())
  }
}
