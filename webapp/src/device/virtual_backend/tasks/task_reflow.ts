import { VirtualBackend, TICK_PERIOD_MS } from '../';
import { Profile, Constants, Point } from '@/proto/generated/types'

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

export function* task_reflow(backend: VirtualBackend, profile: Profile) {
  const timeline = new Timeline()
  let msTime = 0
  timeline.load(profile)

  backend.heat_control_on()

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    backend.remote_history.add({ x: time, y: probe })
    backend.heater.set_temperature(timeline.getTarget(time))
    msTime += TICK_PERIOD_MS

    if (time >= timeline.maxTime) {
      backend.stop()
      return
    }

    yield
  }
}