<script setup lang="ts">
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { inject } from 'vue'
import { Device, DeviceState } from '@/device'
import PageLayout from '@/components/PageLayout.vue'
import HomeMenu from '@/components/HomeMenu.vue'
import ButtonDanger from '@/components/buttons/ButtonDanger.vue'
import ButtonPrimary from '@/components/buttons/ButtonPrimary.vue'
import ReflowChart from '@/components/ReflowChart.vue'

const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()
const device: Device = inject('device')!

async function start() {
  await device.run_reflow()
}

async function stop() {
  await device.stop()
}

</script>

<template>
  <PageLayout>
    <template #toolbar>
      <HomeMenu />

      <div class="mr-2 grow text-ellipsis overflow-hidden whitespace-nowrap">
          {{ profilesStore.selected?.name || '--'}}
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

    <div class="flex-1 flex relative items-center justify-center rounded-md bg-slate-100">
      <div class="absolute top-0 left-0 right-0 bottom-0">
        <ReflowChart id="home-chart"
          :profile="profilesStore.selected"
          :history="device.history.value"
          :show_history="device.history_id.value === profilesStore.selectedId" />
      </div>
      <div v-if="localSettingsStore.showDebugInfo" class="absolute top-2 right-3 text-right text-xs opacity-50">
        <div><span class="font-mono">{{ device.watts.value.toFixed(1) }}</span> W</div>
        <div><span class="font-mono">max {{ Math.round(device.maxWatts.value) }}</span> W</div>
        <div><span class="font-mono">{{ device.volts.value.toFixed(1) }}</span> V</div>
        <div><span class="font-mono">{{ device.amperes.value.toFixed(2) }}</span> A</div>
      </div>
      <div class="absolute bottom-[20%] ">
        <ButtonPrimary
          class="me-1"
          :disabled="device.state.value !== DeviceState.Idle"
          @click="start"
        >
          Start
        </ButtonPrimary>
        <ButtonDanger
          class="ms-1"
          :disabled="device.state.value !== DeviceState.Reflow"
          @click="stop"
        >
          Stop
        </ButtonDanger>
      </div>
    </div>
  </PageLayout>
</template>

<style scoped>
/* :disabled not works for menu elements, use class */
.disabled {
  pointer-events: none;
  opacity: 0.5;
}
</style>
