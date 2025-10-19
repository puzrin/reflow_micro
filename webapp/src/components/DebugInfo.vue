<script setup lang="ts">
import type { DeviceInfo } from '@/proto/generated/types'

defineProps<{
  status: DeviceInfo
}>()
</script>

<template>
  <div>
    <div><span class="font-mono">{{ ((status.peak_mv/1000) * (status.peak_ma/1000) * (status.duty_x1000/1000)).toFixed(2) }} W</span></div>
    <div><span class="font-mono">max {{ (status.max_mw/1000).toFixed(1) }} W</span></div>
    <div><span class="font-mono">{{ (status.peak_mv/1000).toFixed(1) }} V</span></div>
    <div><span class="font-mono">{{ (status.peak_ma/1000).toFixed(2) }} A</span></div>
    <div><span class="font-mono">duty {{ (status.duty_x1000/10).toFixed(0) }} %</span></div>
    <div><span class="font-mono">{{ status.resistance_mohms < 1000*1000 ? (status.resistance_mohms/1000).toFixed(3) : '??' }} Ω</span></div>
    <div><span class="font-mono">{{ (status.temperature_x10/10).toFixed(1) }} °C</span></div>
  </div>
</template>
