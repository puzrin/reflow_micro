<script setup lang="ts">
import { RouterView } from 'vue-router'
import { inject } from 'vue'
import { Device } from '@/device'

const device: Device = inject('device')!

// Views are remounted in two cases:
//
// 1) the route changes (normal behavior)
// 2) the device-ready status changes
//
// The second case is required so page data is fetched correctly after a reload.
</script>

<template>
  <RouterView :key="`${$route.fullPath}-${device.is_ready.value}`" />
</template>
