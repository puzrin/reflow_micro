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
      <v-container class="px-0 py-0">
        <v-toolbar color="transparent" flat tag="div">
          <template v-if="shell.nav.kind === 'menu'" #prepend>
            <HomeMenu />
          </template>

          <template v-else-if="shell.nav.kind === 'back'" #prepend>
            <v-app-bar-nav-icon icon="i-material-symbols:arrow-back" @click="goBack" />
          </template>

          <v-toolbar-title>{{ shell.title }}</v-toolbar-title>

          <template #append>
            <ToolbarIndicator :status="device.status" />
          </template>
        </v-toolbar>
      </v-container>
    </v-app-bar>

    <v-main :class="{ 'd-flex flex-column': shell.pageMode === 'fit-viewport' || !device.is_ready.value }">
      <RouterView v-if="device.is_ready.value" :key="`${$route.fullPath}-${device.is_ready.value}`" />
      <DeviceConnectionInfo v-else />
    </v-main>

    <v-btn
      v-if="device.is_ready.value && (device.backend_id.value === VirtualBackend.id)"
      class="demo-exit"
      rounded="lg"
      color="error"
      variant="flat"
      size="x-small"
      @click="device.selectBackend(BleBackend.id)"
    >
      Exit Demo
    </v-btn>
    <AppConfirmDialog />
    <AppSnackbar />
  </v-app>
</template>

<style scoped>
.demo-exit {
  position: fixed;
  top: 50%;
  right: 0;
  z-index: 3;
  border-end-start-radius: 0;
  border-end-end-radius: 0;
  transform: translate(calc((100% - var(--v-btn-height)) / 2), -50%) rotate(-90deg);
  transform-origin: center;

  font-size: 0.625rem;
  letter-spacing: 0.05em;
}
</style>
