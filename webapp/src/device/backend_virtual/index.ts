import { HeaterModel } from './HeaterModel'
import { PID } from './pid'
import { startTemperature, type Profile } from '@/device/heater_config';
import { useProfilesStore } from '@/stores/profiles'
import { sparsedPush } from '../utils'
import { DeviceState, Device, type IBackend, type Point, HISTORY_ID_RAW_MODE } from '@/device'

// Tick step in ms, 10Hz.
// The real timer interval can be faster, to increase simulation speed.
export const TICK_PERIOD_MS = 100

class Timeline {
  profilePoints: Point[] = [{ x: 0, y: 0 }]

  load(profile: Profile) {
    const profilePoints = [{ x: 0, y: startTemperature }]
    profile.segments.forEach((segment, i) => {
      profilePoints.push({ x: profilePoints[i].x + segment.duration, y: segment.target })
    })
    this.profilePoints = profilePoints
  }

  get maxTime() {
    if (this.profilePoints.length <= 1) return 0
    return this.profilePoints[this.profilePoints.length - 1].x
  }

  getTarget(offset: number) {
    if (offset < 0) return 0

    const points = this.profilePoints

    for (let i = 1; i < this.profilePoints.length; i++) {
      const p0 = points[i - 1]
      const p1 = points[i]

      if (p0.x <= offset && p1.x >= offset) {
        return p0.y + (p1.y - p0.y) / (p1.x - p0.x) * (offset - p0.x)
      }
    }

    return 0
  }
}

export class BackendVirtual implements IBackend {
  private device: Device
  private heater = new HeaterModel()
  private pid = new PID()
  private timeline = new Timeline()
  private msTime = 0

  private state = DeviceState.Idle
  private history_mock: Point[] = []
  private ticker_id: number | null = null

  private static readonly LS_KEY = 'virtual_backend_profiles_data'

  constructor(device: Device) {
    this.device = device
    this.pid
      .setLimits(0, 100)
      .setK(40.0, 100.0, 0.0)
  }

  async fetch_state(): Promise<void> {
    if (!this.device.is_ready.value) return

    this.device.state.value = this.state;
    this.device.temperature.value = this.heater.temperature;
    this.device.watts.value = this.heater.watts;
    this.device.volts.value = this.heater.volts;
    this.device.amperes.value = this.heater.amperes;
    this.device.maxVolts.value = this.heater.maxVolts;
    this.device.maxAmperes.value = this.heater.maxAmperes;
    this.device.maxWatts.value = this.heater.maxWatts;
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
  }

  async connect() {}

  async load_profiles_data(): Promise<string> {
    try {
      const data: any = localStorage?.getItem(BackendVirtual.LS_KEY) || ''
      return data as string
    } catch (_) {}

    return ''
  }

  async save_profiles_data(data: string): Promise<void> {
    try {
      localStorage?.setItem(BackendVirtual.LS_KEY, data)
    } catch (error) {
      console.error('Error saving profiles data:', error);
    }
  }

  private _tick() {
    // Heater emulator works non-stop mode
    this.heater.tick(TICK_PERIOD_MS)

    if (this.state === DeviceState.Running) {
      const time = this.msTime / 1000
      const probe = this.heater.temperature

      sparsedPush(this.history_mock, { x: time, y: probe }, 1.0)

      this.pid
        .setLimits(0, this.heater.maxWatts)
        .setPoint(this.timeline.getTarget(time))

      const watts = this.pid.tick(probe)
      this.heater.setPoint(watts)

      this.msTime += TICK_PERIOD_MS
      // Uncomment to check heater model response & power clamping
      //this.heater.setPoint(100)

      if (time >= this.timeline.maxTime) this.stop()
    }

    if (this.state === DeviceState.RawPower) {
      const time = this.msTime / 1000
      const probe = this.heater.temperature
      this.history_mock.push({ x: time, y: probe })

      this.msTime += TICK_PERIOD_MS
    }
  }

  async start() {
    if (this.state !== DeviceState.Idle) throw new Error('Cannot start profile, device busy')

    const profilesStore = useProfilesStore()
    if (profilesStore.selected === null) throw new Error('No profile set')

    this.device.history.value.length = 0
    this.device.history_id.value = profilesStore.selected.id

    this.msTime = 0
    this.history_mock.length = 0
    this.pid.reset()
    this.timeline.load(profilesStore.selected)
    this.state = DeviceState.Running
  }

  async stop() {
    this.state = DeviceState.Idle
    this.heater.setPoint(0)
  }

  async rawPower(watts: number) {
    if (this.state !== DeviceState.Idle && this.state !== DeviceState.RawPower) {
      throw new Error('Cannot apply direct power, stop profile first')
    }

    if (this.state === DeviceState.Idle) {
      this.device.history.value.length = 0
      this.device.history_id.value = HISTORY_ID_RAW_MODE

      this.msTime = 0
      this.history_mock.length = 0
      this.state = DeviceState.RawPower
    }

    this.heater.setPoint(watts)
  }
}
