<script setup lang="ts">
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { inject } from 'vue'
import { Device } from '@/device'
import { DeviceActivityStatus, HeadStatus, PowerStatus } from '@/proto/generated/types'
import { usePageShell } from '@/composables/appShell'
import ReflowChart from '@/components/ReflowChart.vue'
import DebugInfo from '@/components/DebugInfo.vue'
import HoldToConfirmButton from '@/components/HoldToConfirmButton.vue'

const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()
const device: Device = inject('device')!
const status = device.status

usePageShell(() => ({
  title: profilesStore.selected?.name || '--',
  nav: { kind: 'menu' },
  pageMode: 'fit-viewport',
}))

async function start() {
  await device.run_reflow()
}

async function stop() {
  await device.stop()
}

</script>

<template>
  <v-container class="d-flex flex-column flex-fill py-4 ga-4">
    <v-alert v-if="status.head !== HeadStatus.HeadConnected" class="flex-0-0" type="error">
      Hotplate not connected
    </v-alert>
    <v-alert v-else-if="status.power == PowerStatus.PwrFailure" class="flex-0-0" type="error">
      No suitable power
    </v-alert>

    <v-sheet class="chart-host flex-fill pa-4 rounded border">
      <div class="chart-host-wrap1">
        <div class="chart-host-wrap2">
          <ReflowChart id="home-chart"
            :profile="profilesStore.selected"
            :history="device.history.points"
            :show_history="device.history.id === profilesStore.selectedId"
          />

          <DebugInfo
            v-if="localSettingsStore.showDebugInfo"
            class="chart-host-debug--bottom"
            :status="status"
          />
        </div>
      </div>
    </v-sheet>

    <div class="flex-0-0">
      <HoldToConfirmButton
        v-if="status.activity === DeviceActivityStatus.Idle"
        block
        color="primary"
        variant="elevated"
        @confirm="start"
      >
        Start
      </HoldToConfirmButton>
      <v-btn
        v-else
        block
        color="error"
        variant="elevated"
        @click="stop"
      >
        Stop
      </v-btn>
    </div>
  </v-container>
</template>
