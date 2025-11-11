import type { HeaterControl } from '../heater_control'
import { HeaterTask } from './heater_task'
import { DeviceActivityStatus, Constants } from '@/proto/generated/types'

export class TaskSensorBake extends HeaterTask {
  historyId = Constants.HISTORY_ID_SENSOR_BAKE_MODE
  readonly activityId = DeviceActivityStatus.SensorBake

  constructor(
    private heater: HeaterControl,
    private watts: number
  ) {
    super()
  }

  start(): boolean {
    this.heater.set_power(this.watts)
    return true
  }

  get iterator() {
    return (function* () {
      while (true) {
        yield
      }
    })()
  }
}
