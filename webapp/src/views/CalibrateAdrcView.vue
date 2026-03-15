<script setup lang="ts">
import { onBeforeRouteLeave } from 'vue-router'
import { watchDebounced } from '@vueuse/core'
import { inject, ref, computed, watch } from 'vue'
import { Device } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import HoldToConfirmButton from '@/components/HoldToConfirmButton.vue'
import { HeadParams, DeviceActivityStatus, HeadStatus, Constants } from '@/proto/generated/types'
import { DEFAULT_HEAD_PARAMS_PB } from '@/proto/generated/defaults'
import { useLocalSettingsStore } from '@/stores/localSettings'
import DebugInfo from '@/components/DebugInfo.vue'
import { notify } from '@/composables/notify'
import { usePageShell } from '@/composables/appShell'

const localSettingsStore = useLocalSettingsStore()

const device: Device = inject('device')!

const status = device.status
const activity = computed(() => device.status.activity)
const is_idle = computed(() => activity.value === DeviceActivityStatus.IDLE)
const is_testing = computed(() => activity.value === DeviceActivityStatus.ADRC_TEST)
const is_step_response = computed(() => activity.value === DeviceActivityStatus.STEP_RESPONSE)

usePageShell(() => ({
  title: 'Calibrate temperature controller',
  nav: { kind: 'back', to: { name: 'settings' } },
  pageMode: 'default',
}))

const adrc_param_tau = ref<number | null>(null)
const adrc_param_b0 = ref<number | null>(null)
const adrc_param_n = ref<number | null>(null)
const adrc_param_m = ref<number | null>(null)
const adrc_error_tau = ref(false)
const adrc_error_b0 = ref(false)
const adrc_error_n = ref(false)
const adrc_error_m = ref(false)

const test_temperature = ref(170)
const step_response_power = ref(25)

function toPrecisionNumber(num: number, valuableDigits: number = 2): number {
  // `Number` required to remove scientific notation and trailing zeros
  return Number(num.toPrecision(valuableDigits))
}

function configToRefs(config: HeadParams) {
  adrc_param_tau.value = toPrecisionNumber(config.adrc_response, 3)
  adrc_param_b0.value = toPrecisionNumber(config.adrc_b0, 3)
  adrc_param_n.value = toPrecisionNumber(config.adrc_n_coeff, 3)
  adrc_param_m.value = toPrecisionNumber(config.adrc_m_coeff, 3)
}

watch(
  () => status.head === HeadStatus.HEAD_CONNECTED,
  async (connected) => {
    if (!connected) return
    configToRefs(await device.get_head_params())
  },
  { immediate: true }
)

onBeforeRouteLeave(async () => {
  if (is_testing.value || is_step_response.value) {
    await device.stop()
  }
  return true
})

// Update the temperature on the fly (only while testing is active)
watchDebounced(test_temperature, async () => {
  if (is_testing.value) await device.run_adrc_test(test_temperature.value)
}, { debounce: 500 })

// Reload ADRC settings when any task finishes
watch(activity, async (newState) => {
  if (newState === DeviceActivityStatus.IDLE && device.is_ready.value) {
    configToRefs(await device.get_head_params())
  }
})

async function save_adrc_params() {
  if (adrc_param_tau.value == null) { adrc_error_tau.value = true; return }
  if (adrc_param_b0.value == null) { adrc_error_b0.value = true; return }
  if (adrc_param_n.value == null) { adrc_error_n.value = true; return }
  if (adrc_param_m.value == null) { adrc_error_m.value = true; return }

  adrc_error_tau.value = false
  adrc_error_b0.value = false
  adrc_error_n.value = false
  adrc_error_m.value = false

  try {
    const head_params = await device.get_head_params()
    head_params.adrc_response = adrc_param_tau.value
    head_params.adrc_b0 = adrc_param_b0.value
    head_params.adrc_n_coeff = adrc_param_n.value
    head_params.adrc_m_coeff = adrc_param_m.value
    await device.set_head_params(head_params)

    configToRefs(await device.get_head_params())
    notify({ message: 'Settings saved', color: 'success' })
  } catch {
    notify({ message: 'Failed to save', color: 'error' })
  }
}

async function default_adrc_params() {
  configToRefs(HeadParams.decode(DEFAULT_HEAD_PARAMS_PB))
}

async function runStepResponse() {
  try {
    await device.run_step_response(step_response_power.value)
  } catch {
    notify({ message: 'Failed to run', color: 'error' })
  }
}

async function runAdrcTest() {
  try {
    await device.run_adrc_test(test_temperature.value)
  } catch {
    notify({ message: 'Failed to run', color: 'error' })
  }
}

async function stopTask(force: boolean = false) {
  try {
    await device.stop(force)
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
      <v-card>
        <v-card-item title="Manual ADRC settings" />
        <v-divider />
        <v-card-text>
          <div class="mb-3 text-medium-emphasis">
            Response time for a step input. Run the step response test to detect it.
          </div>
          <v-number-input
            v-model="adrc_param_tau"
            label="τ (sec)"
            inset
            :min="1"
            :max="1000"
            :step="0.001"
            :precision="3"
            :error-messages="adrc_error_tau ? ['Required'] : []"
            @update:model-value="adrc_error_tau = false"
          />

          <div class="mb-3 text-medium-emphasis">
            Controller scale factor. Run the step response test to detect it.
          </div>
          <v-number-input
            v-model="adrc_param_b0"
            label="b0 (scale)"
            inset
            :min="0"
            :max="100"
            :step="0.00001"
            :precision="5"
            :error-messages="adrc_error_b0 ? ['Required'] : []"
            @update:model-value="adrc_error_b0 = false"
          />

          <div class="mb-3 text-medium-emphasis">
            Increase until power jitter starts, then reduce it by 10-20%. ωc = N/τ.
          </div>
          <v-number-input
            v-model="adrc_param_n"
            label="N (controller multiplier)"
            :min="3"
            :max="1000"
            :step="0.5"
            :precision="1"
            :error-messages="adrc_error_n ? ['Required'] : []"
            @update:model-value="adrc_error_n = false"
          />

          <div class="mb-3 text-medium-emphasis">
            Usually 1.5 to 3; start with 2. ωo = M * ωc.
          </div>
          <v-number-input
            v-model="adrc_param_m"
            label="M (observer ratio)"
            inset
            :min="2"
            :max="10"
            :step="0.5"
            :precision="1"
            :error-messages="adrc_error_m ? ['Required'] : []"
            @update:model-value="adrc_error_m = false"
          />
        </v-card-text>
        <v-card-actions>
          <v-btn color="primary" @click="save_adrc_params">Save</v-btn>
          <v-btn @click="default_adrc_params">Load defaults</v-btn>
        </v-card-actions>
      </v-card>

      <v-card>
        <v-card-item title="Measure step response" />
        <v-divider />
        <v-card-text>
          <div class="mb-3 text-medium-emphasis">
            Used to calculate τ and b0. Use the same power you would use for baking.
          </div>
          <v-number-input v-model="step_response_power" label="Power (W)" inset :min="0" :max="100" :step="1" />
        </v-card-text>
        <v-card-actions>
          <HoldToConfirmButton color="primary" @confirm="runStepResponse" :disabled="!is_idle">Run</HoldToConfirmButton>
          <v-btn @click="stopTask()" :disabled="!is_step_response">Stop</v-btn>
        </v-card-actions>
      </v-card>

      <v-card>
        <v-card-item title="Test the controller" />
        <v-divider />
        <v-card-text>
          <v-number-input v-model="test_temperature" label="Temperature (°C)" inset :min="25" :max="300" :step="1" />
        </v-card-text>
        <v-card-actions>
          <HoldToConfirmButton color="primary" @confirm="runAdrcTest" :disabled="!is_idle">Run</HoldToConfirmButton>
          <v-btn @click="stopTask(true)" :disabled="!is_testing">Stop</v-btn>
        </v-card-actions>
      </v-card>

      <v-sheet class="chart-host chart-host--fixed-h flex-fill pa-4 border">
        <div class="chart-host-wrap1">
          <div class="chart-host-wrap2">
            <ReflowChart id="calibrate-adrc-test"
              :profile="null"
              :history="device.history.points"
              :show_history="[Constants.HISTORY_ID_ADRC_TEST_MODE, Constants.HISTORY_ID_STEP_RESPONSE].includes(device.history.id)" />

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
