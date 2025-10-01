<script setup lang="ts">
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink, onBeforeRouteLeave } from 'vue-router'
import { watchDebounced } from '@vueuse/core'
import { inject, onMounted, ref, computed } from 'vue'
import { Device } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import { DeviceActivityStatus, HeadStatus, Constants } from '@/proto/generated/types'

const device: Device = inject('device')!

const saveP0Btn = ref()
const saveP1Btn = ref()

const status = computed(() => device.status.value)
const is_idle = computed(() => status.value.activity === DeviceActivityStatus.Idle)
const is_baking = computed(() => status.value.activity === DeviceActivityStatus.SensorBake)

const p0 = ref('')
const p1 = ref('')
const p0_orig = ref(0)
const p1_orig = ref(0)

const show_p0_error = ref(false)
const show_p1_error = ref(false)
const is_p0_calibrated = computed(() => p0_orig.value > 0)
const is_p1_calibrated = computed(() => p1_orig.value > 0)
const power = ref(50)

async function loadCalibrationStatus() {
  const head_params = await device.get_head_params()
  p0_orig.value = head_params.sensor_p0_temperature
  p1_orig.value = head_params.sensor_p1_temperature
}

onMounted(async () => { await loadCalibrationStatus() })

onBeforeRouteLeave(async () => {
  if (status.value.activity === DeviceActivityStatus.SensorBake) await device.stop()
  return true
})

// Update power "on the fly" (only when baking on progress)
watchDebounced(power, async () => {
  if (status.value.activity === DeviceActivityStatus.SensorBake) await device.run_sensor_bake(toNumber(power.value))
}, { debounce: 500 })

function isNumberLike(val: string | number): boolean {
  if (typeof val === 'number') return true
  return  !isNaN(Number(val.replace(',', '.'))) && val.trim() !== ''
}

function toNumber(val: string | number) {
  if (typeof val === 'number') return val
  return parseFloat(val.replace(',', '.')) || 0
}

async function save_p0() {
  if (p0.value === '') return
  if (!isNumberLike(p0.value)) { show_p0_error.value = true; return }

  show_p0_error.value = false
  const head_params = await device.get_head_params()
  head_params.sensor_p0_temperature = toNumber(p0.value)
  await device.set_head_params(head_params)

  saveP0Btn.value?.showSuccess()
  await loadCalibrationStatus()
  p0.value = ''
}

async function save_p1() {
  if (p1.value === '') return
  if (!isNumberLike(p1.value)) { show_p1_error.value = true; return }

  show_p1_error.value = false
  await device.set_cpoint1(toNumber(p1.value))

  saveP1Btn.value?.showSuccess()
  await loadCalibrationStatus()
  p1.value = ''
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
          :class="status.temperature > 50 ? 'text-red-500' : 'text-green-500'"
        >•</span>
        <span class="font-mono">{{ status.temperature.toFixed(0) }}</span>°C
      </div>
    </template>

    <div v-if="status.head !== HeadStatus.HeadConnected" class="text-red-800 mb-4">
      <p class="text-red-500">Hotplate not connected</p>
    </div>
    <template v-else>
      <p class="text-slate-400 mb-8">
        Set temperature at 2 points for proper sensor calibration.
      </p>


      <h2 class="text-2xl mb-0.5 text-slate-800">Heat point 1 (room temperature)</h2>
      <div v-if="is_p0_calibrated" class="mb-1 text-xs text-green-600">
        <span>Calibrated at {{ Math.round(p0_orig) }}°C</span>
      </div>
      <div v-if="!is_p0_calibrated" class="mb-1 text-xs text-orange-500">
        <span>Not calibrated</span>
      </div>
      <p class="text-sm text-slate-400 mb-4">
        For cold hotplate, set the real value of room temperature.
      </p>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="p0" type="number" inputmode="numeric" min="10" max="100" class="w-full" />
          <ButtonNormal ref="saveP0Btn" @click="save_p0" :disabled="!is_idle">Save</ButtonNormal>
        </div>
        <div v-if="show_p0_error" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
      </div>


      <h2 class="text-2xl mb-0.5 text-slate-800">Heat point 2 (~ 200°C)</h2>
      <div v-if="is_p1_calibrated" class="mb-1 text-xs text-green-600">
        <span>Calibrated at {{ Math.round(p1_orig) }}°C</span>
      </div>
      <div v-if="!is_p1_calibrated" class="mb-1 text-xs text-orange-500">
        <span>Not calibrated</span>
      </div>
      <p class="text-sm text-slate-400 mb-4">
        Select power to get 200-250°C. Wait until temperature become stable, and enter the real value.
        50W is recommended for the start. Adjust if needed. DON'T overheat above 300°C!
      </p>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="power" type="range" min="20" max="100" class="w-full" />
          <ButtonNormal @click="device.run_sensor_bake(toNumber(power))" :disabled="!is_idle">Bake</ButtonNormal>
          <ButtonNormal @click="device.stop()" :disabled="!is_baking">Stop</ButtonNormal>
        </div>
        <div class="text-xs text-slate-400 mt-0.5">Power {{power}}W</div>
      </div>

      <div class="mb-8">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="p1" type="number" inputmode="numeric" min="150" max="300" class="w-full" />
          <ButtonNormal ref="saveP1Btn" @click="save_p1" :disabled="!is_baking">Save</ButtonNormal>
        </div>
        <div v-if="show_p1_error" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
      </div>


      <div class="mt-4 relative rounded-md bg-slate-100 h-[300px]">
        <div class="absolute top-0 left-0 right-0 bottom-0">
          <ReflowChart id="calibrate-sensor-bake"
            :profile="null"
            :history="device.history.value"
            :show_history="device.history_id.value === Constants.HISTORY_ID_SENSOR_BAKE_MODE" />
        </div>
      </div>
    </template>
  </PageLayout>
</template>
