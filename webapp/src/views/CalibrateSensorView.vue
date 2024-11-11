<script setup lang="ts">
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink, onBeforeRouteLeave } from 'vue-router'
import { watchDebounced } from '@vueuse/core'
import { inject, onMounted, ref, computed } from 'vue'
import { Device, HISTORY_ID_SENSOR_BAKE_MODE } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import { DeviceState } from '@/proto/generated/types'

const device: Device = inject('device')!

const saveP1Btn = ref()
const saveP2Btn = ref()

const is_idle = computed(() => device.state.value === DeviceState.Idle)
const is_baking = computed(() => device.state.value === DeviceState.SensorBake)

const p1 = ref('')
const p2 = ref('')
const show_p1_error = ref(false)
const show_p2_error = ref(false)
const is_p1_calibrated = ref(false)
const is_p2_calibrated = ref(false)
const power = ref(50)

async function loadCalibrationStatus() {
  const sensor_params = await device.get_sensor_params()
  is_p1_calibrated.value = !!sensor_params.points[0]
  is_p2_calibrated.value = !!sensor_params.points[1]
}

onMounted(async () => { await loadCalibrationStatus() })

onBeforeRouteLeave(async () => {
  if (device.state.value === DeviceState.SensorBake) await device.stop()
  return true
})

// Update power "on the fly" (only when baking on progress)
watchDebounced(power, async () => {
  if (device.state.value === DeviceState.SensorBake) await device.run_sensor_bake(power.value)
}, { debounce: 500 })

function isNumberLike(val: string | number): boolean {
  if (typeof val === 'number') return true
  return  !isNaN(Number(val.replace(',', '.'))) && val.trim() !== ''
}

function toNumber(val: string | number) {
  if (typeof val === 'number') return val
  return parseFloat(val.replace(',', '.')) || 0
}

async function save_p1() {
  if (p1.value === '') return
  if (!isNumberLike(p1.value)) { show_p1_error.value = true; return }

  show_p1_error.value = false
  await device.set_sensor_calibration_point(0, toNumber(p1.value))
  saveP1Btn.value?.showSuccess()
  await loadCalibrationStatus()
  p1.value = ''
}

async function save_p2() {
  if (p2.value === '') return
  if (!isNumberLike(p2.value)) { show_p2_error.value = true; return }

  show_p2_error.value = false
  await device.set_sensor_calibration_point(1, toNumber(p2.value))
  saveP2Btn.value?.showSuccess()
  await loadCalibrationStatus()
  p2.value = ''
  await device.stop()
}
</script>

<template>
  <PageLayout>
    <template #toolbar>
      <RouterLink :to="{ name: 'settings' }" class="mr-2 -ml-0.5">
        <BackIcon class="w-8 h-8" />
      </RouterLink>
      <div class="mr-2 grow text-ellipsis overflow-hidden whitespace-nowrap">
        Calibrate temperature sensor
      </div>
      <div>
        <span
          class="mr-1"
          :class="device.temperature.value > 50 ? 'text-red-500' : 'text-green-500'"
        >•</span>
        <span class="font-mono">{{ device.temperature.value.toFixed(0) }}</span>°C
      </div>
    </template>

    <div v-if="!device.is_hotplate_ok.value" class="text-red-800 mb-4">
      <p class="text-red-500">Hotplate not connected</p>
    </div>
    <template v-else>
      <p class="text-slate-400 mb-8">
        Set temperature at 2 points for proper sensor calibration.
      </p>


      <h2 class="text-2xl mb-0.5 text-slate-800">Heat point 1 (room temperature)</h2>
      <div v-if="is_p1_calibrated" class="mb-1 text-xs text-green-600">
        <span>Calibrated</span>
      </div>
      <div v-if="!is_p1_calibrated" class="mb-1 text-xs text-orange-500">
        <span>Not calibrated</span>
      </div>
      <p class="text-sm text-slate-400 mb-4">
        For cold hotplate, set the real value of room temperature.
      </p>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="p1" type="number" inputmode="numeric" min="10" max="100" class="w-full" />
          <ButtonNormal ref="saveP1Btn" @click="save_p1" :disabled="!is_idle">Save</ButtonNormal>
        </div>
        <div v-if="show_p1_error" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
      </div>


      <h2 class="text-2xl mb-0.5 text-slate-800">Heat point 2 (~ 200°C)</h2>
      <div v-if="is_p2_calibrated" class="mb-1 text-xs text-green-600">
        <span>Calibrated</span>
      </div>
      <div v-if="!is_p2_calibrated" class="mb-1 text-xs text-orange-500">
        <span>Not calibrated</span>
      </div>
      <p class="text-sm text-slate-400 mb-4">
        Select power to get 200-250°C. Wait until temperature become stable, and enter the real value.
        50W is recommended for the start. Adjust if needed. DON'T overheat above 300°C!
      </p>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="power" type="range" min="20" max="100" class="w-full" />
          <ButtonNormal @click="device.run_sensor_bake(power)" :disabled="!is_idle">Bake</ButtonNormal>
          <ButtonNormal @click="device.stop()" :disabled="!is_baking">Stop</ButtonNormal>
        </div>
        <div class="text-xs text-slate-400 mt-0.5">Power {{power}}W</div>
      </div>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="p2" type="number" inputmode="numeric" min="150" max="300" class="w-full" />
          <ButtonNormal ref="saveP2Btn" @click="save_p2" :disabled="!is_baking">Save</ButtonNormal>
        </div>
        <div v-if="show_p2_error" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
      </div>


      <div class="mt-4 relative rounded-md bg-slate-100 h-[300px]">
        <div class="absolute top-0 left-0 right-0 bottom-0">
          <ReflowChart id="calibrate-sensor-bake"
            :profile="null"
            :history="device.history.value"
            :show_history="device.history_id.value === HISTORY_ID_SENSOR_BAKE_MODE" />
        </div>
      </div>
    </template>
  </PageLayout>
</template>
