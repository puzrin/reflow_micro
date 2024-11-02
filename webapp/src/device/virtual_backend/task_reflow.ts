import { VirtualBackend, TICK_PERIOD_MS } from './';
import { sparsedPush } from './utils';
import { startTemperature, type Profile } from '@/device/heater_config'
import { type Point } from '@/device'
import { useVirtualBackendStore} from './virtualBackendStore'
import { createADRC } from './utils';

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
export function* task_reflow(backend: VirtualBackend, profile: Profile) {
  const timeline = new Timeline()
  let msTime = 0
  backend.history_mock.length = 0
  timeline.load(profile)

  const virtualBackendStore = useVirtualBackendStore()
  const adrc = createADRC(virtualBackendStore.adrc_config)
  adrc.reset_to(backend.heater.temperature)


  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    sparsedPush(backend.history_mock, { x: time, y: probe }, 1.0)

    const watts = adrc.iterate(
      probe,
      timeline.getTarget(time),
      backend.heater.get_max_power(),
      TICK_PERIOD_MS / 1000
    )

    backend.heater.set_power(watts)
    msTime += TICK_PERIOD_MS

    if (time >= timeline.maxTime) {
      backend.stop()
      return
    }

    yield
  }
}