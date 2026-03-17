<script setup lang="ts">
import type { DeviceInfo } from '@/proto/generated/types'
import { SharedConstants as Constants } from '@/lib/shared_constants'

defineProps<{
  status: DeviceInfo
}>()
</script>

<template>
  <v-chip
    :color="status.temperature_x10 < 1000 * 10 ? ((status.temperature_x10 / 10) > Constants.MAX_TOUCH_SAFE_TEMPERATURE ? 'error' : 'success') : 'secondary'"
  >
    <span>
      <template v-if="status.temperature_x10 < 1000 * 10">
        {{ Math.trunc(status.temperature_x10 / 10) }}.{{ Math.abs(status.temperature_x10 % 10) }} °C
      </template>
      <template v-else>
        ?? °C
      </template>
    </span>
  </v-chip>
</template>
