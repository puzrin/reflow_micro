<script setup lang="ts">
import type { DeviceInfo } from '@/proto/generated/types'

defineProps<{
  status: DeviceInfo
}>()
</script>

<template>
  <div>
    <span
      class="mr-1"
      :class="(status.temperature_x10/10) > 50 ? 'text-red-500' : 'text-green-500'"
    >
      •
    </span>
    <span class="font-mono">
      {{ Math.floor(status.temperature_x10 / 10) }}
    </span>
    <span class="text-[0.7em] opacity-80">.</span>
    <span class="font-mono text-[0.7em] opacity-80">
      {{ status.temperature_x10 % 10 }}
    </span>°C
  </div>
</template>
