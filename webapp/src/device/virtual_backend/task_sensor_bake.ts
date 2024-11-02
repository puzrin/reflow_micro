import { VirtualBackend, TICK_PERIOD_MS } from './';

export function* task_sensor_bake(backend: VirtualBackend) {
    let msTime = 0
    backend.history_mock.length = 0

    while (true) {
        backend.history_mock.push({ x: msTime/1000, y: backend.heater.temperature })
        msTime += TICK_PERIOD_MS;
        yield
    }
}