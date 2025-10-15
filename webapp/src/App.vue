<script setup lang="ts">
import { RouterView } from 'vue-router'
import { inject } from 'vue'
import { Device } from '@/device'

const device: Device = inject('device')!

// Views are remounted in 2 cases:
//
// 1) route changes (normal behavior)
// 2) device ready status changed
//
// The second required for proper page data fetch on page reload
</script>

<template>
  <RouterView :key="`${$route.fullPath}-${device.is_ready.value}`" />
</template>
