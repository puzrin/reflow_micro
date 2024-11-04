import { VirtualBackend, TICK_PERIOD_MS } from '../';
import { sparsedPush } from '../utils';

export function* task_sensor_bake(backend: VirtualBackend) {
  let msTime = 0
  backend.history_mock.length = 0

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    sparsedPush(backend.history_mock, { x: time, y: probe })

    msTime += TICK_PERIOD_MS;
    yield
  }
}