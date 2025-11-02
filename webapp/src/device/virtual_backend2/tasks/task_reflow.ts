import type { HeaterControl } from '../heater_control'
import { HeaterTask } from './heater_task'
import { useProfilesStore } from '@/stores/profiles'
import { DeviceActivityStatus, Constants, Point, type Profile } from '@/proto/generated/types'

class Timeline {
  profilePoints: Point[] = [{ x: 0, y: 0 }]

  load(profile: Profile) {
    const profilePoints = [{ x: 0, y: Constants.START_TEMPERATURE }]
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

export class TaskReflow extends HeaterTask {
  historyId = 0
  readonly activityId = DeviceActivityStatus.Reflow

  private timeline = new Timeline()

  constructor(private heater: HeaterControl) {
    super()
  }

  start(): boolean {
    const profilesStore = useProfilesStore()
    const profile = profilesStore.selected
    if (!profile) return false

    this.historyId = profile.id
    this.timeline.load(profile)

    const head_params = this.heater.get_head_params()
    this.heater.temperature_control_on(head_params)

    return true
  }

  get iterator(): Generator<void, void, number> {
    const timeline = this.timeline
    const heater = this.heater

    return (function* () {
      while (true) {
        const task_time_ms: number = yield
        const time_s = task_time_ms / 1000

        if (time_s >= timeline.maxTime) {
          return
        }

        heater.set_temperature(timeline.getTarget(time_s))
      }
    })()
  }
}
