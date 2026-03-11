<script setup lang="ts">
import type { DeviceInfo } from '@/proto/generated/types'

defineProps<{
  status: DeviceInfo
}>()
</script>

<template>
  <div class="text-mono text-label-small text-right opacity-40">
    <div>{{ ((status.peak_mv / 1000) * (status.peak_ma / 1000) * (status.duty_x1000 / 1000)).toFixed(2) }} W</div>
    <div>max {{ (status.max_mw / 1000).toFixed(1) }} W</div>
    <div>{{ (status.peak_mv / 1000).toFixed(2) }} V</div>
    <div>{{ (status.peak_ma / 1000).toFixed(2) }} A</div>
    <div>duty {{ (status.duty_x1000 / 10).toFixed(0) }} %</div>
    <div>{{ status.resistance_mohms < 1000 * 1000 ? (status.resistance_mohms / 1000).toFixed(3) : '??' }} Ω</div>
    <div>{{ status.temperature_x10 < 1000 * 10 ? (status.temperature_x10 / 10).toFixed(1) : '??' }} °C</div>
  </div>
</template>
