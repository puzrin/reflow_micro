import { Heater, configured_heater } from './heater'
import { useProfilesStore } from '@/stores/profiles'
import { useVirtualBackendStore } from './virtualBackendStore'
import { Device, type IBackend,
  HISTORY_ID_SENSOR_BAKE_MODE, HISTORY_ID_ADRC_TEST_MODE, HISTORY_ID_STEP_RESPONSE } from '@/device'
import { task_sensor_bake } from './tasks/task_sensor_bake'
import { task_adrc_test, task_adrc_test_setpoint } from './tasks/task_adrc_test'
import { task_reflow } from './tasks/task_reflow'
import { task_step_response } from './tasks/task_step_response'
import { ProfilesData, Point, AdrcParams, SensorParams, HistoryChunk, DeviceState, HeaterConfigs } from '@/proto/generated/types'
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/profiles_data_pb'
import { DEFAULT_ADRC_PARAMS_PB } from '@/proto/generated/adrc_params_pb'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
  private device: Device
  heater: Heater

  private state = DeviceState.Idle
  history_mock: Point[] = [] // public, to update from tasks

  private ticker_id: ReturnType<typeof setInterval> | null = null

  private static readonly LS_KEY = 'virtual_backend_profiles_data'
  private task_iterator: Generator | null = null

  // Used to simulate remote history sync.
  client_history_version: number = -1
  remote_history_version: number = 0
  remote_history_id: number = 0

  private readonly head_id = 0
  private heater_configs: HeaterConfigs = HeaterConfigs.create()

  constructor(device: Device) {
    this.device = device
    this.heater = configured_heater[0]
      .clone()
      .reset()
      .scale_r_to(4.0)
      .set_size(0.08, 0.07, 0.0028)
      //.set_size(0.07, 0.06, 0.0028)
  }

  async fetch_state(): Promise<void> {
    if (!this.device.is_ready.value) return

    this.device.state.value = this.state;
    this.device.temperature.value = this.heater.temperature;
    this.device.watts.value = this.heater.get_power();
    this.device.volts.value = this.heater.get_volts();
    this.device.amperes.value = this.heater.get_amperes();
    this.device.maxWatts.value = this.heater.get_max_power();
  }

  async fetch_history(): Promise<void> {
    if (!this.device.is_ready.value) return

    const len = this.device.history.value.length
    const after = len ? this.device.history.value[len-1].x : -1

    const history_slice = await this.get_history_slice(this.client_history_version, after)

    if (!history_slice) return

    // Merge update
    if (history_slice.version === this.client_history_version) {
      this.device.history.value.push(...history_slice.data)
      return
    }

    // Full replace
    this.client_history_version = history_slice.version
    this.device.history.value.splice(0, this.device.history.value.length, ...history_slice.data)
    this.device.history_id.value = history_slice.type
  }

  async attach() {
    this.device.is_connecting.value = true
    this.device.is_connected.value = true
    this.device.need_pairing.value = false
    this.device.is_authenticated.value = true
    this.device.is_hotplate_ok.value = true
    this.client_history_version = -1

    await this.device.loadProfilesData()

    // Load heater configs
    const virtualBackendStore = useVirtualBackendStore()
    try {
      this.heater_configs = HeaterConfigs.fromJSON(virtualBackendStore.rawHeaterConfigs)
    } catch {
      console.error('Error loading heater configs, use empty one')
      this.heater_configs = HeaterConfigs.create()
    }

    this.device.is_ready.value = true

    // Increase simulation speed 10x
    this.ticker_id = setInterval(() => this._tick(), TICK_PERIOD_MS / 10)
  }

  async detach() {
    if (this.ticker_id !== null) clearInterval(this.ticker_id)
    if (this.state !== DeviceState.Idle) await this.stop()
    this.history_mock.length = 0
    this.device.is_connecting.value = false
    this.device.is_connected.value = false
    this.device.is_authenticated.value = false
    this.device.is_ready.value = false
    this.device.is_hotplate_ok.value = false
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
    this.heater.set_power(0)
  }

  private reset_remote_history(new_id: number) {
    this.remote_history_id = new_id
    this.remote_history_version++
    this.history_mock.length = 0
  }

  private async get_history_slice(version: number, after: number): Promise<HistoryChunk | null> {
    // History outdated => return full new one
    if (this.remote_history_version != version) {
      return {
        type: this.remote_history_id,
        version: this.remote_history_version,
        data: this.history_mock
      }
    }

    // Calculate diff len, return null if no data to update.
    const diff =  this.history_mock.filter(point => point.x > after)

    if (!diff.length) return null

    return {
      type: this.remote_history_id,
      version: this.remote_history_version,
      data: diff
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
      this.reset_remote_history(HISTORY_ID_SENSOR_BAKE_MODE)
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
      this.reset_remote_history(HISTORY_ID_ADRC_TEST_MODE)
      this.state = DeviceState.AdrcTest
      this.task_iterator = task_adrc_test(this)
    }

    task_adrc_test_setpoint(temperature)
  }

  async run_step_response(watts: number) {
    if (this.state !== DeviceState.Idle) throw new Error('Cannot run test, device busy')

    this.reset_remote_history(HISTORY_ID_STEP_RESPONSE)
    this.state = DeviceState.StepResponse
    this.task_iterator = task_step_response(this, watts)
}

  async set_sensor_calibration_point(point_id: (0 | 1), temperature: number) {
    const hc = this.heater_configs
    if (!hc.sensor[this.head_id]) hc.sensor[this.head_id] = SensorParams.create()

    // Write point with dummy sensor value
    if (temperature === 0) {
      delete hc.sensor[this.head_id].points[point_id]
    } else {
      hc.sensor[this.head_id].points[point_id] = { temperature, sensor_value: 0 }
    }

    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawHeaterConfigs = HeaterConfigs.toJSON(hc) as string
  }

  async get_sensor_params(): Promise<SensorParams> {
    const hc = this.heater_configs

    if (hc.sensor[this.head_id]) return structuredClone(hc.sensor[this.head_id])
    return SensorParams.create()
  }

  async set_adrc_params(config: AdrcParams): Promise<void> {
    const hc = this.heater_configs
    hc.adrc[this.head_id] = structuredClone(config)

    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawHeaterConfigs = HeaterConfigs.toJSON(hc) as string
  }

  // Sync, for local use from tasks
  pick_adrc_params(): AdrcParams {
    const hc = this.heater_configs
    if (hc.adrc[this.head_id]) return structuredClone(hc.adrc[this.head_id])
    return AdrcParams.decode(DEFAULT_ADRC_PARAMS_PB)
  }

  async get_adrc_params(): Promise<AdrcParams> {
    return this.pick_adrc_params()
  }
}
