import { ref } from 'vue'
import { DeviceState, type IDeviceDriver, type Point, TICK_PERIOD_MS } from '../types'
import { HeaterModel } from './HeaterModel'
import { PID } from './pid'
import { startTemperature, type Profile, type ProfilesStoreData } from '@/device/heater_config';
import { useProfilesStore } from '@/stores/profiles'
import { sparsedPush } from '../utils'
import validateProfileStoreSnapshot from '../utils/profiles_store.validate';

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

class VirtualDevice implements IDeviceDriver {
  private heater: HeaterModel
  private pid: PID
  private timeline: Timeline
  public msTime = 0

  private unsubscribeProfilesStore: (() => void) | null = null

  state = ref(DeviceState.Idle)
  history = ref<Point[]>([])

  get temperature() { return this.heater.temperature }
  get resistance() { return this.heater.resistance }
  get watts() { return this.heater.watts }
  get volts() { return this.heater.volts }
  get amperes() { return this.heater.amperes }
  get maxVolts() { return this.heater.maxVolts }
  get maxAmperes() { return this.heater.maxAmperes }
  get maxWatts() { return this.heater.maxWatts }

  constructor() {
    this.timeline = new Timeline()
    this.heater = new HeaterModel()
    this.pid = new PID()
      .setLimits(0, 65)     // 65W max
      //.setK(3.0, 2.0, 0.1)  // Initial values, before calibration fill-in
      //.setK(0.154, 0.385, 0.0205)
      //.setK(0.462, 0.923, 0.0205)
      .setK(40.0, 100.0, 0.0)

    setInterval(() => this._tick(), TICK_PERIOD_MS) // 10Hz
  }

  private _tick() {
    // Heater emulator works non-stop mode
    this.heater.tick()

    if (this.state.value === DeviceState.Running) {
      const time = this.msTime / 1000
      const probe = this.temperature.value

      sparsedPush(this.history.value, { x: time, y: probe }, 1.0)

      this.pid
        .setLimits(0, this.maxWatts.value)
        .setPoint(this.timeline.getTarget(time))

      const watts = this.pid.tick(probe)
      this.heater.setPoint(watts)

      this.msTime += TICK_PERIOD_MS
      // Uncomment to check heater model response & power clamping
      //this.heater.setPoint(100)

      if (time >= this.timeline.maxTime) this.stop()
    }

    if (this.state.value === DeviceState.RawPower) {
      const time = this.msTime / 1000
      const probe = this.temperature.value
      this.history.value.push({ x: time, y: probe })

      this.msTime += TICK_PERIOD_MS
    }
  }

  async start() {
    if (this.state.value === DeviceState.RawPower) {
      throw new Error('Cannot start profile in raw power state')
    }

    const profilesStore = useProfilesStore()

    if (profilesStore.selected === null) {
      throw new Error('No profile set')
    }

    this.msTime = 0
    this.history.value.length = 0
    this.pid.reset()
    this.timeline.load(profilesStore.selected)
    this.state.value = DeviceState.Running
  }

  async stop() {
    this.state.value = DeviceState.Idle
    this.heater.setPoint(0)
  }

  async rawPower(watts: number) {
    if (this.state.value === DeviceState.Running) {
      throw new Error('Cannot apply direct power, stop profile first')
    }

    this.state.value = DeviceState.RawPower
    this.heater.setPoint(watts)
  }

  async attach() {
    // Remove old tracker is exists
    if (this.unsubscribeProfilesStore) this.unsubscribeProfilesStore()

    // Load default profiles data
    const profilesStore = useProfilesStore()
    const LS_KEY = 'virtual_device_profiles_config'

    try {
      const data: any = JSON.parse(localStorage.getItem(LS_KEY) || 'null')
      if (!data) throw new Error('No data, load defaults')

      if (!validateProfileStoreSnapshot(data)) {
        console.error(validateProfileStoreSnapshot.errors)
        throw new Error('Invalid data, load defaults')
      }

      profilesStore.fromRawObj(data as ProfilesStoreData)
    }
    catch (error) {
      console.log('Error loading profiles data:', (error as any)?.message || error)
      profilesStore.reset()
    }

    this.unsubscribeProfilesStore = profilesStore.$subscribe(() => {
      try {
        localStorage.setItem(LS_KEY, JSON.stringify(profilesStore.toRawObj()))
      } catch (error) {
        console.error('Error saving profiles data:', error);
      }
    })
  }

  async shutdown() {
    if (this.state.value !== DeviceState.Idle) await this.stop()
    if (this.unsubscribeProfilesStore) this.unsubscribeProfilesStore()
    this.history.value.length = 0
  }
}

export default VirtualDevice
