<script setup lang="ts">
import { RouterView, useRouter } from 'vue-router'
import { inject } from 'vue'
import { Device } from '@/device'
import AppConfirmDialog from '@/components/AppConfirmDialog.vue'
import AppSnackbar from '@/components/AppSnackbar.vue'
import DeviceConnectionInfo from '@/components/DeviceConnectionInfo.vue'
import HomeMenu from '@/components/HomeMenu.vue'
import ToolbarIndicator from '@/components/ToolbarIndicator.vue'
import { useAppShellState } from '@/composables/appShell'
import { BleBackend } from '@/device/ble_backend'
import { VirtualBackend } from '@/device/virtual_backend'

const device: Device = inject('device')!
const router = useRouter()
const shell = useAppShellState()

function goBack() {
  if (shell.nav.kind === 'back') {
    router.push(shell.nav.to as Parameters<typeof router.push>[0])
  }
}

// Views are remounted in two cases:
//
// 1) the route changes (normal behavior)
// 2) the device-ready status changes
//
// The second case is required so page data is fetched correctly after a reload.
</script>

<template>
  <v-app>
    <v-app-bar v-if="device.is_ready.value">
      <template v-if="shell.nav.kind === 'menu'" #prepend>
        <HomeMenu />
      </template>

      <template v-else-if="shell.nav.kind === 'back'" #prepend>
        <v-app-bar-nav-icon icon="mdi-arrow-left" @click="goBack" />
      </template>

      <v-app-bar-title>{{ shell.title }}</v-app-bar-title>

      <template #append>
        <ToolbarIndicator :status="device.status" />
      </template>
    </v-app-bar>

    <v-main :class="{ 'd-flex flex-column': shell.pageMode === 'fit-viewport' || !device.is_ready.value }">
      <RouterView v-if="device.is_ready.value" :key="`${$route.fullPath}-${device.is_ready.value}`" />
      <DeviceConnectionInfo v-else />
    </v-main>

    <v-btn
      v-if="device.is_ready.value && (device.backend_id.value === VirtualBackend.id)"
      class="demo-exit"
      color="error"
      size="x-small"
      @click="device.selectBackend(BleBackend.id)"
    >
      Exit Demo Mode
    </v-btn>
    <AppConfirmDialog />
    <AppSnackbar />
  </v-app>
</template>

<style scoped>
.demo-exit {
  position: fixed;
  right: 16px;
  bottom: 16px;
  z-index: 3;
}
</style>
