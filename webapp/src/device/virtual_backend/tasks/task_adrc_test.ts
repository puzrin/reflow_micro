import type { HeaterControl } from '../heater_control'
import { HeaterTask } from './heater_task'
import { DeviceActivityStatus, Constants } from '@/proto/generated/types'

export class TaskAdrcTest extends HeaterTask {
  historyId = Constants.HISTORY_ID_ADRC_TEST_MODE
  readonly activityId = DeviceActivityStatus.AdrcTest

  constructor(
    private heater: HeaterControl,
    private temperature: number
  ) {
    super()
  }

  start(): boolean {
    const head_params = this.heater.get_head_params()
    this.heater.temperature_control_on(head_params)
    this.heater.set_temperature(this.temperature)
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
