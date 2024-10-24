import { ref, type App, type Ref } from "vue"
import type { ProfilesStoreData } from "@/device/heater_config"
import { VirtualBackend } from "./virtual_backend"
import { useProfilesStore } from '@/stores/profiles'
import validateProfileStoreSnapshot from './utils/profiles_store.validate'
import { set } from "@vueuse/core"

export enum DeviceState {
  Idle = 0,
  Running = 1,
  RawPower = 2
}

export type Point = { x: number, y: number }

export const HISTORY_ID_RAW_MODE = -1

export interface IBackend {
  attach(): Promise<void>
  detach(): Promise<void>

  start(): Promise<void>
  stop(): Promise<void>
  rawPower(watts: number): Promise<void>

  load_profiles_data(): Promise<string>
  save_profiles_data(data: string): Promise<void>
  fetch_state(): Promise<void>
  fetch_history(): Promise<void>

  // BLE only, to select GATT device from click
  connect(): Promise<void>
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
  temperature: Ref<number> = ref(0)

  // Debug info properties
  watts: Ref<number> = ref(0)
  volts: Ref<number> = ref(0)
  amperes: Ref<number> = ref(0)
  maxVolts: Ref<number> = ref(0)
  maxAmperes: Ref<number> = ref(0)
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
  async start() { await this.backend?.start() }
  async stop() { await this.backend?.stop() }
  async rawPower(watts: number) { await this.backend?.rawPower(watts) }

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
    // Remove old tracker is exists
    this.unsubscribeProfilesStore?.()
    this.unsubscribeProfilesStore = null
    
    if (!this.backend) return;
    
    const profilesStore = useProfilesStore()

    try {
      const raw_data = await this.backend.load_profiles_data()
      if (!raw_data) throw new Error('No data, load defaults')

      const data: any = JSON.parse(raw_data);

      if (!validateProfileStoreSnapshot(data)) {
        console.error(validateProfileStoreSnapshot.errors)
        throw new Error('Invalid data, load defaults')
      }

      profilesStore.fromRawObj(data as ProfilesStoreData)
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
