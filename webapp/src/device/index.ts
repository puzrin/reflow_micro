import Virtual from "./virtual"
import type { IDeviceManager } from "./types"
import { computed, ref, type App } from "vue"

const virtual = new Virtual()

const drivers = {
  virtual
}

type DriverKey = keyof typeof drivers

const driverKey = ref<DriverKey>('virtual')

const device: IDeviceManager = {
  // Driver proxy
  temperature: computed(() => drivers[driverKey.value].temperature.value),
  resistance: computed(() => drivers[driverKey.value].resistance.value),
  watts: computed(() => drivers[driverKey.value].watts.value),
  volts: computed(() => drivers[driverKey.value].volts.value),
  amperes: computed(() => drivers[driverKey.value].amperes.value),
  maxVolts: computed(() => drivers[driverKey.value].maxVolts.value),
  maxAmperes: computed(() => drivers[driverKey.value].maxAmperes.value),
  maxWatts: computed(() => drivers[driverKey.value].maxWatts.value),

  state: computed(() => drivers[driverKey.value].state.value),
  history: computed(() => drivers[driverKey.value].history.value),

  start: () => drivers[driverKey.value].start(),
  stop: () => drivers[driverKey.value].stop(),
  rawPower: (watts) => drivers[driverKey.value].rawPower(watts),

  // Control
  id: computed(() => driverKey.value),
  select: async (id: DriverKey) => {
    if (driverKey.value !== id) {
      // Detach old device first
      await drivers[driverKey.value as DriverKey].shutdown()
    }

    driverKey.value = id
    await drivers[id].attach()
  },
  resetHistory: () => {
    drivers[driverKey.value].history.value.length = 0
    drivers[driverKey.value].msTime = 0
  }
}

export default {
  install: (app: App) => {
    app.provide('device', device)
    device.select('virtual')
  }
}
