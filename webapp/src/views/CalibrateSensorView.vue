<script setup lang="ts">
import { onBeforeRouteLeave } from 'vue-router'
import { watchDebounced } from '@vueuse/core'
import { inject, ref, computed, watch } from 'vue'
import { Device } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import HoldToConfirmButton from '@/components/HoldToConfirmButton.vue'
import { DeviceActivityStatus, HeadStatus } from '@/proto/generated/types'
import { SharedConstants as Constants } from '@/lib/shared_constants'
import { SENSOR_CALIBRATION_LIMITS } from '@/lib/web_limits'
import { useLocalSettingsStore } from '@/stores/localSettings'
import DebugInfo from '@/components/DebugInfo.vue'
import { notify } from '@/composables/notify'
import { usePageShell } from '@/composables/appShell'

const localSettingsStore = useLocalSettingsStore()

const device: Device = inject('device')!

const status = device.status
const is_idle = computed(() => status.activity === DeviceActivityStatus.IDLE)
const is_baking = computed(() => status.activity === DeviceActivityStatus.SENSOR_BAKE)

usePageShell(() => ({
  title: 'Calibrate temperature sensor',
  nav: { kind: 'back', to: { name: 'settings' } },
  pageMode: 'default',
}))

const p0 = ref<number | null>(null)
const p1 = ref<number | null>(null)
const p0_orig = ref(0)
const p1_orig = ref(0)

const show_p0_error = ref(false)
const show_p1_error = ref(false)
const is_p0_calibrated = computed(() => p0_orig.value > 0)
const is_p1_calibrated = computed(() => p1_orig.value > 0)
const power = ref(25)

async function loadCalibrationStatus() {
  const head_params = await device.get_head_params()
  p0_orig.value = head_params.sensor_p0_at
  p1_orig.value = head_params.sensor_p1_at
}

watch(
  () => status.head === HeadStatus.HEAD_CONNECTED,
  async (connected) => {
    if (!connected) return
    await loadCalibrationStatus()
  },
  { immediate: true }
)

onBeforeRouteLeave(async () => {
  if (is_baking.value) await device.stop()
  return true
})

// Update power on the fly (only while baking is in progress)
watchDebounced(power, async () => {
  if (is_baking.value) await device.run_sensor_bake(power.value)
}, { debounce: 500 })

async function save_p0() {
  if (p0.value == null) { show_p0_error.value = true; return }

  show_p0_error.value = false
  try {
    await device.set_cpoint0(p0.value)
    await loadCalibrationStatus()
    p0.value = null
  } catch {
    notify({ message: 'Failed to save', color: 'error' })
  }
}

async function save_p1() {
  if (p1.value == null) { show_p1_error.value = true; return }

  show_p1_error.value = false
  try {
    await device.set_cpoint1(p1.value)
    await loadCalibrationStatus()
    p1.value = null
  } catch {
    notify({ message: 'Failed to save', color: 'error' })
    return
  }

  try {
    await device.stop(true)
  } catch {
    notify({ message: 'Failed to stop', color: 'error' })
  }
}

async function startBake() {
  try {
    await device.run_sensor_bake(power.value)
  } catch {
    notify({ message: 'Failed to start baking', color: 'error' })
  }
}

async function stopBake() {
  try {
    await device.stop()
  } catch {
    notify({ message: 'Failed to stop', color: 'error' })
  }
}
</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-alert v-if="status.head !== HeadStatus.HEAD_CONNECTED" type="error">
      Hotplate not connected
    </v-alert>
    <template v-else>
      <v-alert>
        Set the temperature at two points to calibrate the sensor properly.
      </v-alert>

      <v-card>
        <v-card-item title="Point 1 (low)">
          <template #append>
            <v-chip :color="is_p0_calibrated ? 'success' : 'warning'" variant="outlined" size="x-small">
              {{ is_p0_calibrated ? `at ${Math.round(p0_orig)}°C` : 'not set' }}
            </v-chip>
          </template>
        </v-card-item>
        <v-divider />
        <v-card-text>
          <div class="mb-3 text-medium-emphasis">
            With a cold hotplate, enter the actual room temperature.
          </div>
          <v-number-input
            v-model="p0"
            label="Temperature (°C)"
            inset
            :min="SENSOR_CALIBRATION_LIMITS.point0Min"
            :max="SENSOR_CALIBRATION_LIMITS.point0Max"
            :step="SENSOR_CALIBRATION_LIMITS.point0Step"
            :precision="1"
            :error-messages="show_p0_error ? ['Required'] : []"
            @update:model-value="show_p0_error = false"
          />
        </v-card-text>
        <v-card-actions>
          <v-btn color="primary" @click="save_p0" :disabled="!is_idle">Save</v-btn>
        </v-card-actions>
      </v-card>

      <v-card>
        <v-card-item title="Point 2 (high)">
          <template #append>
            <v-chip :color="is_p1_calibrated ? 'success' : 'warning'" variant="outlined" size="x-small">
              {{ is_p1_calibrated ? `at ${Math.round(p1_orig)}°C` : 'not set' }}
            </v-chip>
          </template>
        </v-card-item>
        <v-divider />
        <v-card-text>
          <div class="mb-3 text-medium-emphasis">
            Select a power level that stays below the maximum supported temperature
            (~25 W / 170°C for the MCPCB heater, ~50 W / 250°C for the MCH-based heater).
            Wait until the temperature stabilizes, then enter the actual value.
          </div>
          <v-slider
            v-model="power"
            :min="SENSOR_CALIBRATION_LIMITS.bakePowerMin"
            :max="SENSOR_CALIBRATION_LIMITS.bakePowerMax"
            thumb-label="always"
          >
            <template v-slot:thumb-label="{ modelValue }">
              {{ Math.round(modelValue * 10)/10 }}&nbsp;W
            </template>
          </v-slider>

          <div class="mb-3 text-medium-emphasis">
            Enter the actual value once the temperature becomes stable.
          </div>
          <v-number-input
            v-model="p1"
            label="Temperature (°C)"
            inset
            :min="SENSOR_CALIBRATION_LIMITS.point1Min"
            :max="SENSOR_CALIBRATION_LIMITS.point1Max"
            :step="SENSOR_CALIBRATION_LIMITS.point1Step"
            :precision="1"
            :disabled="!is_baking"
            :error-messages="show_p1_error ? ['Required'] : []"
            @update:model-value="show_p1_error = false"
          />
        </v-card-text>
        <v-card-actions>
          <HoldToConfirmButton color="error" @confirm="startBake" :disabled="!is_idle">Bake</HoldToConfirmButton>
          <v-btn @click="stopBake" :disabled="!is_baking">Stop</v-btn>
          <v-btn color="primary" @click="save_p1" :disabled="!is_baking">Save</v-btn>
        </v-card-actions>
      </v-card>

      <v-sheet class="chart-host chart-host--fixed-h flex-fill pa-4 border">
        <div class="chart-host-wrap1">
          <div class="chart-host-wrap2">
            <ReflowChart id="calibrate-sensor-bake"
              :profile="null"
              :history="device.history.points"
              :show_history="device.history.id === Constants.HISTORY_ID_SENSOR_BAKE_MODE" />

            <DebugInfo
              v-if="localSettingsStore.showDebugInfo"
              class="chart-host-debug--bottom"
              :status="status"
            />
          </div>
        </div>
      </v-sheet>
    </template>
  </v-container>
</template>
