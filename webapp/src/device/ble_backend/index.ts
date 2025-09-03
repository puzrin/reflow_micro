import { Device, type IBackend } from '@/device'
import { ProfilesData, HeadParams, HistoryChunk, DeviceStatus, Constants } from '@/proto/generated/types'
import { BleRpcClient } from '../../../src/lib/ble/BleRpcClient';

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
    this.bleRpcClient.log = (...data) => { console.log(...data); };
    this.bleRpcClient.log_error = (...data) => { console.error(...data); };

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
    const from = len ? this.device.history.value[len-1].x : 0

    const pb_history_chunk: Uint8Array = await this.bleRpcClient.invoke('get_history_chunk', this.client_history_version, from) as Uint8Array
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
      await this.device.loadProfilesData()
      this.config_data_loaded = true
      this.device.is_ready.value = true
      await this.fetch_status()
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

  async get_head_params(): Promise<HeadParams> {
    const pb_head_params: Uint8Array = await this.bleRpcClient.invoke('get_head_params') as Uint8Array
    return HeadParams.decode(pb_head_params)
  }

  async set_head_params(config: HeadParams): Promise<void> {
    this.bleRpcClient.invoke('set_head_params', HeadParams.encode(config).finish())
  }
}
