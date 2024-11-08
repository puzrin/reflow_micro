import { VirtualBackend, TICK_PERIOD_MS } from '../';
import { sparsedPush } from '../utils';
import { useVirtualBackendStore} from '../virtualBackendStore'
import { createADRC } from '../utils';

let setpoint = 0

export function task_adrc_test_setpoint(value: number) { setpoint = value }

export function* task_adrc_test(backend: VirtualBackend) {
  let msTime = 0
  backend.history_mock.length = 0

  const virtualBackendStore = useVirtualBackendStore()
  const adrc = createADRC(virtualBackendStore.adrc_config)
  adrc.reset_to(backend.heater.temperature)

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    sparsedPush(backend.history_mock, { x: time, y: probe }, 1.0)

    const watts = adrc.iterate(
      probe,
      setpoint,
      backend.heater.get_max_power(),
      TICK_PERIOD_MS / 1000
    )

    backend.heater.set_power(watts)
    msTime += TICK_PERIOD_MS

    yield
  }
}