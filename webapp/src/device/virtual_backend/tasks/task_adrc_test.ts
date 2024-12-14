import { VirtualBackend, TICK_PERIOD_MS } from '../';

export function* task_adrc_test(backend: VirtualBackend) {
  let msTime = 0
  backend.remote_history.reset()

  backend.heat_control_on()

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    backend.remote_history.add({ x: time, y: probe })

    msTime += TICK_PERIOD_MS
    yield
  }
}