import { VirtualBackend, TICK_PERIOD_MS } from '../';

export function* task_sensor_bake(backend: VirtualBackend) {
  let msTime = 0
  backend.remote_history.reset()

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    backend.remote_history.add({ x: time, y: probe })

    msTime += TICK_PERIOD_MS;
    yield
  }
}