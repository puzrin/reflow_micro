import { Heater, configured_heater } from './heater'
import { useProfilesStore } from '@/stores/profiles'
import { useVirtualBackendStore } from './virtualBackendStore'
import { Device, History, type IBackend } from '@/device'
import { task_sensor_bake } from './tasks/task_sensor_bake'
import { task_adrc_test } from './tasks/task_adrc_test'
import { task_reflow } from './tasks/task_reflow'
import { task_step_response } from './tasks/task_step_response'
import { ProfilesData, AdrcParams, SensorParams, HistoryChunk, DeviceState, HeaterParams, Constants } from '@/proto/generated/types'
import { DEFAULT_PROFILES_DATA_PB, DEFAULT_HEATER_PARAMS_PB } from '@/proto/generated/defaults'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
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
  remote_history: History = new History() // public, to update from tasks

  private heater_params: HeaterParams = HeaterParams.create()

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
      duty_cycle: 1
    }
  }

  async fetch_history(): Promise<void> {
    if (!this.device.is_ready.value) return

    const len = this.device.history.value.length
    const after = len ? this.device.history.value[len-1].x : -1

    const history_slice = await this.get_history_slice(this.client_history_version, after)

    if (!history_slice) return

    if (history_slice.version === this.client_history_version) {
      // Merge update
      History.merge(this.device.history.value, history_slice.data, this.remote_history.precision)
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

    // Load heater configs
    const virtualBackendStore = useVirtualBackendStore()
    try {
      this.heater_params = HeaterParams.fromJSON(virtualBackendStore.rawHeaterParams)
    } catch {
      console.error('Error loading heater configs, use default one')
      this.heater_params = HeaterParams.decode(DEFAULT_HEATER_PARAMS_PB)
    }

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
    const defObj = ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)

    if (reset) {
      await this.save_profiles_data(defObj)
      return await this.load_profiles_data(false)
    }

    let data: ProfilesData;

    try {
      data = ProfilesData.fromJSON(virtualBackendStore.rawProfilesData)
    } catch (error) {
      console.error('Error loading profiles data:', (error as Error).message || error)
      await this.save_profiles_data(defObj)
      data = defObj
    }

    return data
  }

  async save_profiles_data(data: ProfilesData): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawProfilesData = ProfilesData.toJSON(data) as string
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

  private async get_history_slice(version: number, after: number): Promise<HistoryChunk> {
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
      data: this.remote_history.get_data_after(after).slice(0, Constants.MAX_HISTORY_CHUNK)
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
    // This is not required since we always use pre-defined values. Left to pass types check.
    if (!this.heater_params.sensor) this.heater_params.sensor = SensorParams.create()

    if (point_id === 0) this.heater_params.sensor.p0_temperature = temperature
    else this.heater_params.sensor.p1_temperature = temperature

    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawHeaterParams = HeaterParams.toJSON(this.heater_params) as string
  }

  async get_sensor_params(): Promise<SensorParams> {
    return structuredClone(this.heater_params.sensor || SensorParams.create())
  }

  async set_adrc_params(config: AdrcParams): Promise<void> {
    this.heater_params.adrc = structuredClone(config)

    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawHeaterParams = HeaterParams.toJSON(this.heater_params) as string
  }

  // Sync, for local use from tasks
  pick_adrc_params(): AdrcParams {
    return structuredClone(this.heater_params.adrc || AdrcParams.create())
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
