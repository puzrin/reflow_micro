<script setup lang="ts">
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { inject } from 'vue'
import { Device } from '@/device'
import { DeviceActivityStatus } from '@/proto/generated/types'
import PageLayout from '@/components/PageLayout.vue'
import HomeMenu from '@/components/HomeMenu.vue'
import ButtonDanger from '@/components/buttons/ButtonDanger.vue'
import ButtonPrimary from '@/components/buttons/ButtonPrimary.vue'
import ReflowChart from '@/components/ReflowChart.vue'
import DebugInfo from '@/components/DebugInfo.vue'
import ToolbarIndicator from '@/components/ToolbarIndicator.vue'

const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()
const device: Device = inject('device')!
const status = device.status

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
      <ToolbarIndicator :status="status" />
    </template>

    <div class="mb-4 flex-1 flex relative items-center justify-center rounded-md bg-slate-100">
      <div class="absolute top-0 left-0 right-0 bottom-0">
        <ReflowChart id="home-chart"
          :profile="profilesStore.selected"
          :history="device.history.value"
          :show_history="device.history_id.value === profilesStore.selectedId" />
      </div>
      <DebugInfo
        v-if="localSettingsStore.showDebugInfo"
        class="absolute top-2 right-3 text-right text-xs opacity-50"
        :status="status"
      />
    </div>

    <div class="flex justify-center">
      <ButtonPrimary
        class="me-1 mb-0"
        :disabled="status.activity !== DeviceActivityStatus.Idle"
        @click="start"
      >
        Start
      </ButtonPrimary>
      <ButtonDanger
        class="ms-1 mb-0"
        :disabled="status.activity !== DeviceActivityStatus.Reflow"
        @click="stop"
      >
        Stop
      </ButtonDanger>
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
