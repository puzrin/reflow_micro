import { Heater, configured_heater } from './heater'
import { useProfilesStore } from '@/stores/profiles'
import { useVirtualBackendStore } from './virtualBackendStore'
import { Device, type IBackend } from '@/device'
import { SparseHistory } from '@/device/sparse_history'
import { task_sensor_bake } from './tasks/task_sensor_bake'
import { task_adrc_test } from './tasks/task_adrc_test'
import { task_reflow } from './tasks/task_reflow'
import { task_step_response } from './tasks/task_step_response'
import { ProfilesData, AdrcParams, SensorParams, HistoryChunk, DeviceState, Constants } from '@/proto/generated/types'
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/defaults'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
  static id: string = 'virtual' as const

  private device: Device
  heater: Heater

  private state = DeviceState.Idle

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
      state: this.state,
      hotplate_connected: true,
      hotplate_id: 0,
      temperature: this.heater.temperature,
      watts: this.heater.get_power(),
      volts: this.heater.get_volts(),
      amperes: this.heater.get_amperes(),
      max_watts: this.heater.get_max_power(),
      duty_cycle: 1,
      resistance: this.heater.get_resistance()
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

    this.device.status.value.hotplate_connected = true
    this.device.status.value.hotplate_id = 0

    await this.device.loadProfilesData()

    this.device.is_ready.value = true

    // Increase simulation speed 10x
    this.ticker_id = setInterval(() => this._tick(), TICK_PERIOD_MS / 10)
  }

  async detach() {
    if (this.ticker_id !== null) clearInterval(this.ticker_id)
    if (this.state !== DeviceState.Idle) await this.stop()
    this.remote_history.reset()
    this.device.is_connecting.value = false
    this.device.is_connected.value = false
    this.device.is_authenticated.value = false
    this.device.is_ready.value = false

    this.device.status.value.hotplate_connected = false
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
    this.state = DeviceState.Idle
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
    if (this.state !== DeviceState.Idle) throw new Error('Cannot start profile, device busy')

    const profilesStore = useProfilesStore()
    if (profilesStore.selected === null) throw new Error('No profile set')

    this.reset_remote_history(profilesStore.selected.id)
    this.state = DeviceState.Reflow

    this.task_iterator = task_reflow(this, profilesStore.selected)
    this.task_iterator.next()
  }

  async run_sensor_bake(watts: number) {
    if (this.state !== DeviceState.Idle && this.state !== DeviceState.SensorBake) {
      throw new Error('Cannot heat sensor, device busy')
    }

    if (this.state === DeviceState.Idle) {
      this.reset_remote_history(Constants.HISTORY_ID_SENSOR_BAKE_MODE)
      this.state = DeviceState.SensorBake
      this.task_iterator = task_sensor_bake(this)
    }

    this.heater.set_power(watts)
  }

  async run_adrc_test(temperature: number) {
    if (this.state !== DeviceState.Idle && this.state !== DeviceState.AdrcTest) {
      throw new Error('Cannot run test, device busy')
    }

    if (this.state === DeviceState.Idle) {
      this.reset_remote_history(Constants.HISTORY_ID_ADRC_TEST_MODE)
      this.state = DeviceState.AdrcTest
      this.task_iterator = task_adrc_test(this)
    }

    this.heater.set_temperature(temperature)
  }

  async run_step_response(watts: number) {
    if (this.state !== DeviceState.Idle) throw new Error('Cannot run test, device busy')

    this.reset_remote_history(Constants.HISTORY_ID_STEP_RESPONSE)
    this.state = DeviceState.StepResponse
    this.task_iterator = task_step_response(this, watts)
  }

  async set_sensor_calibration_point(point_id: (0 | 1), temperature: number) {
    const sensor_params = await this.get_sensor_params()

    if (point_id === 0) sensor_params.p0_temperature = temperature
    else sensor_params.p1_temperature = temperature

    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawSensorParams = SensorParams.toJSON(sensor_params)
  }

  async get_sensor_params(): Promise<SensorParams> {
    const virtualBackendStore = useVirtualBackendStore()
    return structuredClone(SensorParams.fromJSON(virtualBackendStore.rawSensorParams))
  }

  async set_adrc_params(config: AdrcParams): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawAdrcParams = AdrcParams.toJSON(config)
  }

  // Sync, for local use from tasks
  pick_adrc_params(): AdrcParams {
    const virtualBackendStore = useVirtualBackendStore()
    return structuredClone(AdrcParams.fromJSON(virtualBackendStore.rawAdrcParams))
  }

  async get_adrc_params(): Promise<AdrcParams> {
    return this.pick_adrc_params()
  }

  heat_control_on() {
    const adrc_params = this.pick_adrc_params()
    this.heater.adrc.set_params(adrc_params.b0, adrc_params.response, adrc_params.N, adrc_params.M)
    this.heater.temperature_control_on()
  }

  heat_control_off() {
    this.heater.temperature_control_off()
    this.heater.set_temperature(this.heater.get_room_temp())
  }
}
