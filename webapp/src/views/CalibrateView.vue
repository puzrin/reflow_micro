<script setup lang="ts">
import { until } from '@vueuse/core'
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink } from 'vue-router'
import { inject, ref } from 'vue'
import { Device, type Point, HISTORY_ID_RAW_MODE } from '@/device'
import ReflowChart from '@/components/ReflowChart.vue'
import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import ButtonDanger from '@/components/buttons/ButtonDanger.vue'
import ConfirmDialog from '@/components/ConfirmDialog.vue'

const device: Device = inject('device')!
const pidTuneRunning = ref(false)

const presets = {
  zieglerNichols: [0.6, 0.5, 0.125],
  tyreusLuyben: [0.9, 0.3, 0.2],
  cianconeMarlin: [0.95, 0.3, 0.2],
  chienHronesReswick: [0.7, 0.4, 0.15],
  cohenCoon: [0.9, 0.3, 0.3],
  pessenIntegral: [1.2, 0.4, 0.4],
  someOvershoot: [0.33, 0.33, 0.33],
  noOvershoot: [0.2, 0.4, 0.133],
  forDisturbances: [0.95, 0.3, 0.1],
  forFastSettling: [0.6, 0.5, 0.15],
  forSlowSettling: [0.2, 0.4, 0.133],
}

async function tunePid() {
  pidTuneRunning.value = true

  // Wait cooling down if needed
  if (device.temperature.value > 90) {
    await device.rawPower(0.01)
    await until(device.temperature).toMatch(t => t < 90)
  }

  await device.rawPower(65)
  await until(device.temperature).toMatch(t => t > 100)
  const pBegin: Point = device.history.value[device.history.value.length - 1]

  await until(device.temperature).toMatch(t => t > 200)
  const pMax: Point = device.history.value[device.history.value.length - 1]
  const maxPower = device.watts.value

  await device.rawPower(0.01)
  await until(device.temperature).toMatch(t => t < 100)
  const pEnd: Point = device.history.value[device.history.value.length - 1]

  const Ku = 4 * ((maxPower - 0)/2) / (Math.PI * (pMax.y - pEnd.y)/2)
  const Tu = (pEnd.x - pBegin.x)
  console.log(`Ku = ${Ku.toFixed(4)}, Tu = ${Tu.toFixed(2)}`)

  {
    const [kpConstant, tiConstant, tdConstant] = presets.zieglerNichols
    const Kp = kpConstant * Ku
    const Ki = Kp / tiConstant
    const Kd = Kp * tdConstant
    console.log(`zieglerNichols: Kp = ${Kp.toFixed(3)}, Ki =${Ki.toFixed(3)}, Kd = ${Kd.toFixed(4)}`)
  }
  {
    const [kpConstant, tiConstant, tdConstant] = presets.noOvershoot
    const Kp = kpConstant * Ku
    const Ki = Kp / tiConstant
    const Kd = Kp * tdConstant
    console.log(`noOvershoot: Kp = ${Kp.toFixed(3)}, Ki =${Ki.toFixed(3)}, Kd = ${Kd.toFixed(4)}`)
  }

  device.stop()
  pidTuneRunning.value = false
}
</script>

<template>
  <PageLayout>
    <template #toolbar>
      <RouterLink :to="{ name: 'home' }" class="mr-2 -ml-0.5">
        <BackIcon class="w-8 h-8" />
      </RouterLink>
      <div class="mr-2 grow text-ellipsis overflow-hidden whitespace-nowrap">
        Calibration
      </div>
      <div>
        <span
          class="mr-1"
          :class="device.temperature.value > 50 ? 'text-red-500' : 'text-green-500'"
        >
          •
        </span>
        <span class="font-mono">{{ device.temperature.value.toFixed(0) }}</span>°C
      </div>
    </template>

    <h2 class="mb-4 text-xl font-semibold">Temperature calibration</h2>
    <!--<p class="text-xs text-slate-400 mb-4">
      Temperature at two heat points required for proper operation.
      ~100°C and ~250°C are recommended. Please apply suggested power and
      enter measured result.
    </p>-->

    <h3 class="mb-2 font-medium text-lg text-slate-500">Heat point 1</h3>

    <div class="mb-4">
      <div class="mb-1 text-xs">
        <span>4.567 Ω / 135 °C</span>
      </div>

      <div class="flex gap-4 w-full">
        <div class="flex-1">
          <div class="flex gap-2 flex-nowrap w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              class="w-full"
            />
            <ButtonNormal>On</ButtonNormal>
          </div>
          <div class="text-xs text-slate-400 mt-0.5">Power, W</div>
        </div>
        <div class="flex-1">
          <div class="flex gap-2 flex-nowrap w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              class="w-full"
            />
            <ButtonNormal>Save</ButtonNormal>
          </div>
          <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
        </div>
      </div>
    </div>


    <h3 class="mb-2 font-medium text-lg text-slate-500">Heat point 2</h3>

    <div class="mb-4">
      <div class="mb-1 text-xs">
        <span>4.567 Ω / 135 °C</span>
      </div>

      <div class="flex gap-4 w-full">
        <div class="flex-1">
          <div class="flex gap-2 flex-nowrap w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              class="w-full"
            />
            <ButtonNormal>On</ButtonNormal>
          </div>
          <div class="text-xs text-slate-400 mt-0.5">Power, W</div>
        </div>
        <div class="flex-1">
          <div class="flex gap-2 flex-nowrap w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              class="w-full"
            />
            <ButtonNormal>Save</ButtonNormal>
          </div>
          <div class="text-xs text-slate-400 mt-0.5">Temperature, °C</div>
        </div>
      </div>
    </div>


    <h2 class="mb-2 text-lg font-semibold">PID</h2>

    <div>
      <ButtonNormal @click="tunePid">Auto-tune</ButtonNormal>

      <div class="mb-4 relative">
        <Transition name="bounce">
          <div v-if="pidTuneRunning" class="mt-4 relative rounded-md bg-slate-100 h-[300px]">
            <div class="absolute top-0 left-0 right-0 bottom-0">
              <ReflowChart id="profile-edit-chart"
                :profile="null"
                :history="device.history.value"
                :show_history="device.history_id.value === HISTORY_ID_RAW_MODE" />
            </div>
          </div>
        </Transition>
        <Transition name="bounce">
          <div  v-if="pidTuneRunning" class="absolute top-2 right-3 text-right text-xs opacity-50">
            <div><span class="font-mono">{{ device.watts.value.toFixed(1) }}</span> W</div>
            <div><span class="font-mono">max {{ Math.round(device.maxWatts.value) }}</span> W</div>
            <div><span class="font-mono">{{ device.volts.value.toFixed(1) }}</span> V</div>
            <div><span class="font-mono">{{ device.amperes.value.toFixed(2) }}</span> A</div>
          </div>
        </Transition>
      </div>
    </div>

  </PageLayout>

  <ConfirmDialog ref="clibrationResetDlgRef" v-slot="{ closeAs }">
    <p class="mt-2">Confirm data reset?</p>
    <div class="mt-4">
      <ButtonDanger class="me-2" @click="closeAs('ok')">Yes</ButtonDanger>
      <ButtonNormal class="me-2" @click="closeAs('cancel')">Cancel</ButtonNormal>
    </div>
  </ConfirmDialog>
</template>
