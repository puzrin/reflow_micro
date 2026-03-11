<script setup lang="ts">
import { computed, inject, ref } from 'vue'
import { Device } from '@/device'
import { VirtualBackend } from '@/device/virtual_backend'

const device: Device = inject('device')!
const has_bluetooth = ref(!!navigator.bluetooth)

const reloadPage = () => window.location.reload()

const connectButtonLabel = computed(() => {
  if (device.is_connecting.value && !device.is_connected.value) return 'Connecting…'
  if (device.is_connected.value && !device.is_authenticated.value) return 'Authenticating…'
  if (device.is_connected.value && device.is_authenticated.value && !device.is_ready.value) return 'Reading config…'
  return 'Connect to device'
})

const showConnectSpinner = computed(() => connectButtonLabel.value !== 'Connect to device')
</script>

<template>
  <div class="connection-gate d-flex flex-column flex-fill align-center justify-center pa-4">
    <template v-if="!has_bluetooth">
      <v-card class="w-100" max-width="40rem">
        <v-card-text class="pa-6 pa-sm-8">
          <div class="d-flex flex-column ga-4">
            <v-btn color="primary" variant="text" @click="device.selectBackend(VirtualBackend.id)">
              Switch to Demo Mode
            </v-btn>

            <v-alert type="error">
              WebBluetooth not supported. Use Chrome / Edge, or switch to demo mode.
            </v-alert>

            <v-alert>
              <div class="mb-2">
                On Linux, Chrome / Edge browsers have disabled WebBluetooth by default.
                To enable, follow the steps below:
              </div>
              <ol>
                <li>Open <code>chrome://flags</code> or <code>edge://flags</code>.</li>
                <li>Search for <code>bluetooth</code>.</li>
                <li>Enable all related flags.</li>
                <li>Restart the browser.</li>
              </ol>
            </v-alert>

          </div>
        </v-card-text>
      </v-card>
    </template>

    <template v-else>
      <v-card class="w-100" max-width="40rem">
        <v-card-text class="pa-6 pa-sm-8">
          <div class="d-flex flex-column ga-4">
            <v-btn color="primary" @click="device.connect()">
              <v-progress-circular
                v-if="showConnectSpinner"
                class="mr-3"
                indeterminate
                :size="16"
                :width="2"
              />
              {{ connectButtonLabel }}
            </v-btn>

            <v-alert
              v-if="device.need_pairing.value && device.is_connected.value && !device.is_authenticated.value"
              type="error"
              variant="outlined"
            >
              Device is connected, but not paired. Click the device button five times quickly to enter pairing mode.
            </v-alert>

            <div class="d-flex flex-column flex-sm-row ga-4">
              <v-btn class="flex-1-1-0" variant="text" @click="device.selectBackend(VirtualBackend.id)">
                Switch to Demo Mode
              </v-btn>
              <v-btn class="flex-1-1-0" variant="text" @click="reloadPage">
                Reload Page
              </v-btn>
            </div>
          </div>
        </v-card-text>
      </v-card>
    </template>
  </div>
</template>

<style scoped>
.connection-gate {
  background-color: rgb(0 0 0 / 0.32);
}
</style>
