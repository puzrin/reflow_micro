import type { Ref } from "vue"

export const TICK_PERIOD_MS = 100

export enum DeviceState {
  Idle = 0,
  Running = 1,
  RawPower = 2
}

export type Point = { x: number, y: number }

export interface IHeaterProperties {
  temperature: Ref<number>
  watts: Ref<number>
  resistance: Ref<number>
  volts: Ref<number>
  amperes: Ref<number>
  maxVolts: Ref<number>
  maxAmperes: Ref<number>
  maxWatts: Ref<number>
}

export interface IHeaterModel extends IHeaterProperties {
  setPoint(watts: number): void
  tick(): void
}

export interface IDeviceProperties extends IHeaterProperties {
  state: Ref<DeviceState>
  history: Ref<Point[]>
}

export interface IDeviceMethods {
  start(): Promise<void>
  stop(): Promise<void>
  rawPower(watts: number): Promise<void>
}

export interface IDeviceDriver extends IDeviceProperties, IDeviceMethods {
  attach(): Promise<void>
  shutdown(): Promise<void>
}

export interface IDeviceManager extends IDeviceProperties, IDeviceMethods {
  id: Ref<string>
  select(id: string): void
  resetHistory(): void
}