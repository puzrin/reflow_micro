import { ref, type App, type Ref, toValue } from "vue"
import { VirtualBackend } from "./virtual_backend"
import { BleBackend } from "./ble_backend"
import { useProfilesStore } from '@/stores/profiles'
import { ProfilesData, Point, AdrcParams, SensorParams, DeviceStatus } from '@/proto/generated/types'
import { SparseHistory } from './sparse_history'
import { useLocalSettingsStore } from "@/stores/localSettings"

export type backendIdType = typeof VirtualBackend.id | typeof BleBackend.id

export interface IBackend {
  // init
  attach(): Promise<void>
  detach(): Promise<void>
  connect(): Promise<void> // BLE only, to select GATT device from click

  // tasks
  run_reflow(): Promise<void>
  run_sensor_bake(watts: number): Promise<void>
  run_adrc_test(temperature: number): Promise<void>
  run_step_response(watts: number): Promise<void>
  stop(): Promise<void>

  load_profiles_data(reset: boolean): Promise<ProfilesData>
  save_profiles_data(data: ProfilesData): Promise<void>
  fetch_status(): Promise<void>
  fetch_history(): Promise<void>

  set_sensor_calibration_point(point_id: (0 | 1), value: number): Promise<void>
  get_sensor_params(): Promise<SensorParams>
  set_adrc_params(config: AdrcParams): Promise<void>
  get_adrc_params(): Promise<AdrcParams>
}

export class Device {
  // Connection flags
  is_connecting: Ref<boolean> = ref(false)
  is_connected: Ref<boolean> = ref(false)
  is_authenticated: Ref<boolean> = ref(false)
  need_pairing: Ref<boolean> = ref(false)
  is_ready: Ref<boolean> = ref(false) // connected + authenticated + configs fetched

  status: Ref<DeviceStatus> = ref(DeviceStatus.create({ hotplate_connected: false }))

  history: Ref<Point[]> = ref<Point[]>([])
  history_id: Ref<number> = ref(0)
  // wrapper to aggregate this.history data
  sparseHistory: SparseHistory = SparseHistory.from(this.history.value)

  private backend: IBackend | null = null
  backend_id = ref('')
  private unsubscribeProfilesStore: (() => void) | null = null

  private available_backends = {
    [VirtualBackend.id]: new VirtualBackend(this),
    [BleBackend.id]: new BleBackend(this)
  }

  private timer_call_in_progress: boolean = false

  constructor() {
    setInterval(async () => {
      if (this.timer_call_in_progress) return
      try {
        this.timer_call_in_progress = true
        await this.backend?.fetch_status()
        await this.backend?.fetch_history()
      }
      catch {}
      finally {
        this.timer_call_in_progress = false
      }
    }, 1000)
  }


  async connect() { await this.backend?.connect() }

  async run_reflow() { await this.backend?.run_reflow() }
  async run_sensor_bake(watts: number) { await this.backend?.run_sensor_bake(watts) }
  async run_adrc_test(temperature: number) { await this.backend?.run_adrc_test(temperature) }
  async run_step_response(watts: number) { await this.backend?.run_step_response(watts) }
  async stop() { await this.backend?.stop() }

  async set_sensor_calibration_point(point_id: (0 | 1), value: number) {
    if (!this.backend) throw Error('No backend selected')
    await this.backend.set_sensor_calibration_point(point_id, value)
  }
  async get_sensor_params(): Promise<SensorParams> {
    if (!this.backend) throw Error('No backend selected')
    return await this.backend.get_sensor_params()
  }
  async set_adrc_params(config: AdrcParams) {
    if (!this.backend) throw Error('No backend selected')
    await this.backend.set_adrc_params(config)
  }
  async get_adrc_params(): Promise<AdrcParams> {
    if (!this.backend) throw Error('No backend selected')
    return await this.backend.get_adrc_params()
  }


  // Control
  // id: computed(() => driverKey.value);
  async selectBackend(id: backendIdType) {
    // Don't reselect the same backend
    if (this.backend_id.value === id) return;
    // Detach old one if exists
    if (this.backend) await this.backend.detach();

    // On wrong value restore from localStore - adjust to default
    if (!this.available_backends[id]) id = BleBackend.id;

    // Attach new one
    this.backend = this.available_backends[id];
    this.backend_id.value = id;
    await this.backend.attach();

    // Remember selection
    const localStore = useLocalSettingsStore()
    localStore.backend = id
  };

  async loadProfilesData(reset: boolean = false) {
    // Remove old tracker if exists
    this.unsubscribeProfilesStore?.()
    this.unsubscribeProfilesStore = null

    if (!this.backend) return;

    const profilesStore = useProfilesStore()

    profilesStore.$state = await this.backend.load_profiles_data(reset)

    this.unsubscribeProfilesStore = profilesStore.$subscribe(() => {
      this.backend?.save_profiles_data(toValue(profilesStore.$state))
    })
  }
}

export default {
  install: (app: App) => {
    const device = new Device()

    app.provide('device', device)
    const localStore = useLocalSettingsStore()
    device.selectBackend(localStore.backend)
  }
}
