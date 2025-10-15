<script setup lang="ts">
import DeviceConnectionInfo from '@/components/DeviceConnectionInfo.vue'
import { inject } from 'vue'
import { Device } from '@/device'
import { BleBackend } from '@/device/ble_backend'
import { VirtualBackend } from '@/device/virtual_backend'

const device: Device = inject('device')!
</script>

<template>
  <div class="flex relative flex-col min-h-screen w-full max-w-screen-md mx-auto bg-white">
    <div v-if="device.is_ready.value" class="flex items-center px-4 h-16 shrink-0 text-2xl text-slate-400">
      <slot name="toolbar"></slot>
    </div>
    <div v-if="device.is_ready.value" class="flex-1 flex flex-col px-4 pb-4 text-slate-700 grow">
      <slot></slot>
    </div>

    <div v-if="device.is_ready.value && (device.backend_id.value === VirtualBackend.id)" class="fixed bottom-1 right-2">
        <button
            type="button"
            class="text-white border border-red-700 bg-red-700 font-medium rounded-lg text-xs px-3 py-1.5 text-center me-2 mb-2"
            @click="device.selectBackend(BleBackend.id)"
        >
            Exit Demo Mode
        </button>
    </div>

    <DeviceConnectionInfo />
  </div>
</template>
