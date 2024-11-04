import { Heater, configured_heater } from './heater'
import { useProfilesStore } from '@/stores/profiles'
import { useVirtualBackendStore} from './virtualBackendStore'
import { DeviceState, Device, type IBackend, type Point,
  HISTORY_ID_SENSOR_BAKE_MODE, HISTORY_ID_ADRC_TEST_MODE, HISTORY_ID_STEP_RESPONSE } from '@/device'
import { task_sensor_bake } from './tasks/task_sensor_bake'
import { task_adrc_test, task_adrc_test_setpoint } from './tasks/task_adrc_test'
import { task_reflow } from './tasks/task_reflow'
import { task_step_response } from './tasks/task_step_response'
import type { AdrcConfig } from '../adrc_config'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class VirtualBackend implements IBackend {
  private device: Device
  heater: Heater

  private state = DeviceState.Idle
  history_mock: Point[] = [] // public, to update from tasks
  private ticker_id: number | null = null

  private static readonly LS_KEY = 'virtual_backend_profiles_data'
  private task_iterator: Generator | null = null

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

    this.device.history.value.splice(0, this.device.history.value.length, ...this.history_mock)
  }

  async attach() {
    this.device.is_connecting.value = true
    this.device.is_connected.value = true
    this.device.need_pairing.value = false
    this.device.is_authenticated.value = true
    this.device.is_hotplate_ok.value = true

    await this.device.loadProfilesData()

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

  async load_profiles_data(): Promise<string> {
    const virtualBackendStore = useVirtualBackendStore()
    return virtualBackendStore.rawProfilesData
  }

  async save_profiles_data(data: string): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.rawProfilesData = data
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

  async run_reflow() {
    if (this.state !== DeviceState.Idle) throw new Error('Cannot start profile, device busy')

    const profilesStore = useProfilesStore()
    if (profilesStore.selected === null) throw new Error('No profile set')

    this.device.history.value.length = 0
    this.device.history_id.value = profilesStore.selected.id
    this.state = DeviceState.Reflow

    this.task_iterator = task_reflow(this, profilesStore.selected)
    this.task_iterator.next()
  }

  async run_sensor_bake(watts: number) {
    if (this.state !== DeviceState.Idle && this.state !== DeviceState.SensorBake) {
      throw new Error('Cannot heat sensor, device busy')
    }

    if (this.state === DeviceState.Idle) {
      this.device.history.value.length = 0
      this.device.history_id.value = HISTORY_ID_SENSOR_BAKE_MODE
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
      this.device.history.value.length = 0
      this.device.history_id.value = HISTORY_ID_ADRC_TEST_MODE
      this.state = DeviceState.AdrcTest
      this.task_iterator = task_adrc_test(this)
    }

    task_adrc_test_setpoint(temperature)
  }

  async run_step_response(watts: number) {
    if (this.state !== DeviceState.Idle) throw new Error('Cannot run test, device busy')

    this.device.history.value.length = 0
    this.device.history_id.value = HISTORY_ID_STEP_RESPONSE
    this.state = DeviceState.StepResponse
    this.task_iterator = task_step_response(this, watts)
}

  async set_sensor_calibration_point(point_id: (0 | 1), value: number) {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.sensor_calibration_status[point_id] = value !== 0
  }

  async get_sensor_calibration_status(): Promise<[boolean, boolean]> {
    const virtualBackendStore = useVirtualBackendStore()
    return virtualBackendStore.sensor_calibration_status
  }

  async set_adrc_config(config: AdrcConfig): Promise<void> {
    const virtualBackendStore = useVirtualBackendStore()
    virtualBackendStore.adrc_config = config
  }

  async get_adrc_config(): Promise<AdrcConfig> {
    const virtualBackendStore = useVirtualBackendStore()
    return virtualBackendStore.adrc_config
  }
}
