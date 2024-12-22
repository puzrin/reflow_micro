import { Device, type IBackend } from '@/device'
import { ProfilesData, AdrcParams, SensorParams, HistoryChunk, DeviceStatus, Constants } from '@/proto/generated/types'
import { BleRpcClient } from '../../../src/lib/ble/BleRpcClient';

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

export class BleBackend implements IBackend {
  static id: string = 'ble' as const

  private device: Device
  private bleRpcClient: BleRpcClient = new BleRpcClient()

  client_history_version: number = -1
  is_selected: boolean = false

  config_data_loaded: boolean = false

  constructor(device: Device) {
    this.device = device
    this.bleRpcClient.on('status_changed', () => this.pick_connector_status())
  }

  async fetch_status(): Promise<void> {
    if (!this.device.is_ready.value) return

    const pb_status: Uint8Array = await this.bleRpcClient.invoke('get_status') as Uint8Array

    if (!this.is_selected) return;

    this.device.status.value = DeviceStatus.decode(pb_status)
  }

  async fetch_history(): Promise<void> {
    if (!this.device.is_ready.value) return

    const len = this.device.history.value.length
    const after = len ? this.device.history.value[len-1].x : -1

    const pb_history_chunk: Uint8Array = await this.bleRpcClient.invoke('get_history_chunk', this.client_history_version, after) as Uint8Array
    const history_chunk = HistoryChunk.decode(pb_history_chunk)

    if (history_chunk.version === this.client_history_version) {
      // Merge update
      this.device.sparseHistory.add(...history_chunk.data)
    } else {
      // Full replace
      this.client_history_version = history_chunk.version
      this.device.history.value.splice(0, this.device.history.value.length, ...history_chunk.data)
      this.device.history_id.value = history_chunk.type
    }

    // If data size is max allowed => it could be shrinked => repeat request
    if (history_chunk.data.length >= Constants.MAX_HISTORY_CHUNK) {
      await this.fetch_history()
    }
  }

  private async pick_connector_status() {
    if (!this.is_selected) return

    this.device.is_connected.value = this.bleRpcClient.isConnected()
    this.device.is_authenticated.value = this.bleRpcClient.isAuthenticated()
    this.device.need_pairing.value = this.bleRpcClient.needPairing()

    this.device.is_connecting.value = this.bleRpcClient.isDeviceSelected() && !this.bleRpcClient.isConnected()

    if (this.bleRpcClient.isReady()) {
      if (this.config_data_loaded) return
      await this.load_profiles_data()
      await this.fetch_status()
      this.config_data_loaded = true
      this.device.is_ready.value = true
    } else {
      this.config_data_loaded = false
      this.device.is_ready.value = false
    }
  }

  async attach() {
    this.is_selected = true
    this.client_history_version = -1
    this.config_data_loaded = false

    // Call explicit, to cover re-attach case when device already connected
    await this.pick_connector_status()
  }

  async detach() {
    this.is_selected = false
  }

  async connect() {
    await this.bleRpcClient.selectDevice()
  }

  async load_profiles_data(reset = false): Promise<ProfilesData> {
    const pb_profiles_data: Uint8Array = await this.bleRpcClient.invoke('get_profiles_data', reset) as Uint8Array

    return ProfilesData.decode(pb_profiles_data)
  }

  async save_profiles_data(data: ProfilesData): Promise<void> {
    await this.bleRpcClient.invoke('save_profiles_data', ProfilesData.encode(data).finish())
  }

  async stop() { this.bleRpcClient.invoke('stop') }

  async run_reflow() {
    this.bleRpcClient.invoke('run_reflow')
  }

  async run_sensor_bake(watts: number) {
    this.bleRpcClient.invoke('run_sensor_bake', watts)
  }

  async run_adrc_test(temperature: number) {
    this.bleRpcClient.invoke('run_adrc_test', temperature)
  }

  async run_step_response(watts: number) {
    this.bleRpcClient.invoke('run_step_response', watts)
  }

  async set_sensor_calibration_point(point_id: (0 | 1), temperature: number) {
    this.bleRpcClient.invoke('set_sensor_calibration_point', point_id, temperature)
  }

  async get_sensor_params(): Promise<SensorParams> {
    const pb_sensor_params: Uint8Array = await this.bleRpcClient.invoke('get_sensor_params') as Uint8Array
    return SensorParams.decode(pb_sensor_params)
  }

  async set_adrc_params(config: AdrcParams): Promise<void> {
    this.bleRpcClient.invoke('set_adrc_params', AdrcParams.encode(config).finish())
  }

  async get_adrc_params(): Promise<AdrcParams> {
    const pb_adrc_params: Uint8Array = await this.bleRpcClient.invoke('get_adrc_params') as Uint8Array
    return AdrcParams.decode(pb_adrc_params)
  }
}
