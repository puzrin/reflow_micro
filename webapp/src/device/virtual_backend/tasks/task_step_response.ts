import { VirtualBackend, TICK_PERIOD_MS } from '../';
import { sparsedPush } from '../utils';
import { useVirtualBackendStore } from '../virtualBackendStore';

export function* task_step_response(backend: VirtualBackend, watts: number) {
  let msTime = 0
  backend.history_mock.length = 0
  const virtualBackendStore = useVirtualBackendStore()

  // ts of next log storage
  let next_record_ts = 0

  const temperature_log: number[] = []
  const power_log: number[] = []

  backend.heater.set_power(watts)

  while (true) {
    const time = msTime / 1000
    const probe = backend.heater.temperature

    sparsedPush(backend.history_mock, { x: time, y: probe }, 1.0)

    // Every second
    if (msTime >= next_record_ts) {
      next_record_ts +=1000

      temperature_log.push(probe)
      power_log.push(backend.heater.get_power())

      const len = temperature_log.length

      if (len > 10) {
        // Check until temperature becomes stable (changes < 0.1C/sec)
        if (Math.abs(temperature_log[len-1] - temperature_log[len-10]) < 1) break
      }
    }

    msTime += TICK_PERIOD_MS;
    yield
  }

  const temperature_63 = temperature_log[temperature_log.length-1] * 0.63
  const time_63 = temperature_log.findIndex(val => val >= temperature_63)

  const b0 = (temperature_63 - temperature_log[0]) / time_63 / watts

  console.log(`Temperature = ${temperature_63}, time = ${time_63}, b0 = ${b0}`)

  virtualBackendStore.adrc_config.response = time_63
  virtualBackendStore.adrc_config.b0 = b0
  backend.stop()
}