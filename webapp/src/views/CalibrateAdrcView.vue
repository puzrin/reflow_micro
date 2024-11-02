<script setup lang="ts">
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink, onBeforeRouteLeave } from 'vue-router'
import { watchDebounced } from '@vueuse/core'
import { inject, onMounted, ref, computed } from 'vue'
import { Device, DeviceState, HISTORY_ID_ADRC_TEST_MODE } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import { type AdrcConfig, defaultAdrcConfig } from '@/device/adrc_config'

const device: Device = inject('device')!

const is_idle = computed(() => device.state.value === DeviceState.Idle)
const is_testing = computed(() => device.state.value === DeviceState.AdrcTest)

const adrc_param_tau = ref('')
const adrc_param_b0 = ref('')
const adrc_param_n = ref('')
const adrc_param_m = ref('')
const adrc_error_tau = ref(false)
const adrc_error_b0 = ref(false)
const adrc_error_n = ref(false)
const adrc_error_m = ref(false)

const test_temperature = ref(50)

function configToRefs(config: AdrcConfig) {
  adrc_param_tau.value = config.response.toString()
  adrc_param_b0.value = config.b0.toString()
  adrc_param_n.value = config.N.toString()
  adrc_param_m.value = config.M.toString()
}

onMounted(async () => {
  const adrc_config = await device.get_adrc_config()
  configToRefs(adrc_config)
})

onBeforeRouteLeave(async () => {
  if (device.state.value === DeviceState.SensorBake) await device.stop()
  return true
})

// Update temperature "on the fly" (only when testing active)
watchDebounced(test_temperature, async () => {
  if (device.state.value === DeviceState.AdrcTest) await device.run_adrc_test(test_temperature.value)
}, { debounce: 500 })

function isNumberLike(val: string | number): boolean {
  if (typeof val === 'number') return true
  return  !isNaN(Number(val.replace(',', '.'))) && val.trim() !== ''
}

function toNumber(val: string | number) {
  if (typeof val === 'number') return val
  return parseFloat(val.replace(',', '.')) || 0
}

async function save_adrc_params() {
  if (!isNumberLike(adrc_param_tau.value)) { adrc_error_tau.value = true; return }
  if (!isNumberLike(adrc_param_b0.value)) { adrc_error_b0.value = true; return }
  if (!isNumberLike(adrc_param_n.value)) { adrc_error_n.value = true; return }
  if (!isNumberLike(adrc_param_m.value)) { adrc_error_m.value = true; return }

  adrc_error_tau.value = false
  adrc_error_b0.value = false
  adrc_error_n.value = false
  adrc_error_m.value = false

  const adrc_config: AdrcConfig = {
    response: toNumber(adrc_param_tau.value),
    b0: toNumber(adrc_param_b0.value),
    N: toNumber(adrc_param_n.value),
    M: toNumber(adrc_param_m.value),
  }
  await device.set_adrc_config(adrc_config)
}

async function default_adrc_params() { configToRefs(defaultAdrcConfig) }
</script>

<template>
  <PageLayout>
    <template #toolbar>
      <RouterLink :to="{ name: 'settings' }" class="mr-2 -ml-0.5">
        <BackIcon class="w-8 h-8" />
      </RouterLink>
      <div class="mr-2 grow text-ellipsis overflow-hidden whitespace-nowrap">
        Calibrate temperature controller
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
      <h2 class="text-2xl mb-4 mt-4 text-slate-800">Manual ADRC settings</h2>

      <div class="mb-2">
        <p class="text-base text-slate-800 mb-0.5">τ, sec</p>
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="adrc_param_tau" type="number" inputmode="numeric" min="1" max="1000" class="w-full" />
        </div>
        <div v-if="adrc_error_tau" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-sm text-slate-400 mt-0.5">
          Response time for step input. Time to reach 63% of final value  on step response.
        </div>
      </div>

      <div class="mb-2">
        <p class="text-base text-slate-800 mb-0.5">b0 (scale)</p>
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="adrc_param_b0" type="number" inputmode="numeric" min="0" max="100" step="0.00001" class="w-full" />
        </div>
        <div v-if="adrc_error_b0" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-sm text-slate-400 mt-0.5">
          Max(𝑑T)/ΔPower on step response.
        </div>
      </div>

      <div class="mb-2">
        <p class="text-base text-slate-800 mb-0.5">N (ω<sub>observer</sub> multiplier)</p>
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="adrc_param_n" type="number" inputmode="numeric" min="3" max="50" step="0.5" class="w-full" />
        </div>
        <div v-if="adrc_error_n" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-sm text-slate-400 mt-0.5">
          ω<sub>o</sub> = N/τ. Usually 3..10, but can be more. Increase until oscillation starts, then reduce 10-20%.
        </div>
      </div>

      <div class="mb-4">
        <p class="text-base text-slate-800 mb-0.5">M (ω<sub>controller</sub> ratio)</p>
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="adrc_param_m" type="number" inputmode="numeric" min="2" max="10" step="0.5" class="w-full" />
        </div>
        <div v-if="adrc_error_m" class="text-xs text-red-500 mt-0.5">Not a number</div>
        <div class="text-sm text-slate-400 mt-0.5">
          ω<sub>c</sub> = ω<sub>o</sub>/M. Usually 2..5. With high probability 3 will be ok, and not required to change.
        </div>
      </div>

      <div class="mb-8">
        <ButtonNormal @click="save_adrc_params" class="mr-2">Save</ButtonNormal>
        <ButtonNormal @click="default_adrc_params">Load default params</ButtonNormal>
      </div>


      <h2 class="text-2xl mb-4 text-slate-800">Test controller</h2>
      <div class="mb-2">
        <div class="flex gap-2 flex-nowrap w-full">
          <input v-model="test_temperature" type="range" min="25" max="300" class="w-full" />
          <ButtonNormal @click="device.run_adrc_test(test_temperature)" :disabled="!is_idle">Run</ButtonNormal>
          <ButtonNormal @click="device.stop()" :disabled="!is_testing">Stop</ButtonNormal>
        </div>
        <div class="text-xs text-slate-400 mt-0.5">Temperature {{test_temperature}}°C</div>
      </div>

      <div class="mt-4 relative rounded-md bg-slate-100 h-[300px]">
        <div class="absolute top-0 left-0 right-0 bottom-0">
          <ReflowChart id="profile-edit-chart"
            :profile="null"
            :history="device.history.value"
            :show_history="device.history_id.value === HISTORY_ID_ADRC_TEST_MODE" />
        </div>
      </div>
    </template>
  </PageLayout>
</template>
