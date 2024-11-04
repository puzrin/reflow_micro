import { ref, type App, type Ref } from "vue"
import Ajv from "ajv"
import { default as profiles_schema } from './profiles_schema.json'
import { VirtualBackend } from "./virtual_backend"
import { useProfilesStore } from '@/stores/profiles'
import { type AdrcConfig } from "./adrc_config"

const validate_profiles_data = new Ajv().compile(profiles_schema)

export enum DeviceState {
  Idle = 0,
  Reflow = 1,
  SensorBake = 2,
  AdrcTest = 3,
  StepResponse = 4
}

export type Point = { x: number, y: number }

export const HISTORY_ID_SENSOR_BAKE_MODE = -1
export const HISTORY_ID_ADRC_TEST_MODE = -2
export const HISTORY_ID_STEP_RESPONSE = -3

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

  load_profiles_data(): Promise<string>
  save_profiles_data(data: string): Promise<void>
  fetch_state(): Promise<void>
  fetch_history(): Promise<void>

  set_sensor_calibration_point(point_id: (0 | 1), value: number): Promise<void>
  get_sensor_calibration_status(): Promise<[boolean, boolean]>
  set_adrc_config(config: AdrcConfig): Promise<void>
  get_adrc_config(): Promise<AdrcConfig>
}

export class Device {
  // Connection flags
  is_connecting: Ref<boolean> = ref(false)
  is_connected: Ref<boolean> = ref(false)
  is_authenticated: Ref<boolean> = ref(false)
  need_pairing: Ref<boolean> = ref(false)
  is_ready: Ref<boolean> = ref(false) // connected + authenticated + configs fetched

  // Essential properties
  state: Ref<DeviceState> = ref(DeviceState.Idle)
  is_hotplate_ok: Ref<boolean> = ref(false)
  temperature: Ref<number> = ref(0)

  // Debug info properties
  watts: Ref<number> = ref(0)
  volts: Ref<number> = ref(0)
  amperes: Ref<number> = ref(0)
  maxWatts: Ref<number> = ref(0)

  history: Ref<Point[]> = ref<Point[]>([])
  history_id: Ref<number> = ref(0)

  is_virtual: Ref<boolean> = ref(true)
  private backend: IBackend | null = null
  private backend_id: string = ''
  private unsubscribeProfilesStore: (() => void) | null = null

  constructor() {
    setInterval(async () => {
      try {
        await this.backend?.fetch_state()
        await this.backend?.fetch_history()
      } catch (_) {}
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
  async get_sensor_calibration_status(): Promise<[boolean, boolean]> {
    if (!this.backend) throw Error('No backend selected')
    return await this.backend.get_sensor_calibration_status()
  }
  async set_adrc_config(config: AdrcConfig) {
    if (!this.backend) throw Error('No backend selected')
    await this.backend.set_adrc_config(config)
  }
  async get_adrc_config(): Promise<AdrcConfig> {
    if (!this.backend) throw Error('No backend selected')
    return await this.backend.get_adrc_config()
  }


  // Control
  // id: computed(() => driverKey.value);
  async selectBackend(id: BackendKey) {
    // Don't reselect the same backend
    if (this.backend_id === id) return;
    // Detach old one if exists
    if (this.backend) await this.backend.detach();

    // Attach new one
    this.is_virtual.value = id === 'virtual';
    this.backend = backends[id];
    this.backend_id = id;
    await this.backend.attach();
  };

  async loadProfilesData() {
    // Remove old tracker if exists
    this.unsubscribeProfilesStore?.()
    this.unsubscribeProfilesStore = null

    if (!this.backend) return;

    const profilesStore = useProfilesStore()

    try {
      const raw_data = await this.backend.load_profiles_data()
      if (!raw_data) throw new Error('No data, load defaults')

      const data: any = JSON.parse(raw_data);

      if (!validate_profiles_data(data)) {
        console.error(validate_profiles_data.errors)
        throw new Error('Invalid data, load defaults')
      }

      profilesStore.fromRawObj(data as any)
    }
    catch (error) {
      console.error('Error loading profiles data:', (error as any)?.message || error)
      profilesStore.reset()
      this.backend?.save_profiles_data(JSON.stringify(profilesStore.toRawObj()))
    }

    this.unsubscribeProfilesStore = profilesStore.$subscribe(() => {
      this.backend?.save_profiles_data(JSON.stringify(profilesStore.toRawObj()))
    })
  }
}

const device = new Device()

const backends = {
  virtual: new VirtualBackend(device)
}

type BackendKey = keyof typeof backends

export default {
  install: (app: App) => {
    app.provide('device', device)
    device.selectBackend('virtual')
  }
}
