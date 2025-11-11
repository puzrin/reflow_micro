import type { DeviceActivityStatus } from '@/proto/generated/types'

/**
 * Base class for heater tasks. Mirrors firmware FSM state architecture.
 */
export abstract class HeaterTask {
  abstract historyId: number
  abstract readonly activityId: DeviceActivityStatus

  // Called once before task starts. Return false to abort.
  abstract start(): boolean

  // Generator receives task_time_ms via yield
  abstract readonly iterator: Generator<void, void, number>
}
