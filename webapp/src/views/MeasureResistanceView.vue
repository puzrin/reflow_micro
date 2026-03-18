<script setup lang="ts">
import { computed, inject } from 'vue'
import { Device } from '@/device'
import { PowerStatus } from '@/proto/generated/types'
import { usePageShell } from '@/composables/appShell'

const device: Device = inject('device')!

const status = device.status

usePageShell(() => ({
  title: 'Measure resistance',
  nav: { kind: 'back', to: { name: 'settings' } },
  pageMode: 'default',
}))

const hasPowerError = computed(() => status.power === PowerStatus.PWR_FAILURE)
const hasResistanceValue = computed(() => status.resistance_mohms < 1000 * 1000)

const resistanceText = computed(() => {
  if (!hasResistanceValue.value) return (0).toFixed(3)
  return `${(status.resistance_mohms / 1000).toFixed(3)} Ω`
})
</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-alert
      v-if="hasPowerError"
      type="warning"
      text="Power error"
    />

    <v-card>
      <v-card-text class="py-10">
        <div class="measure-resistance text-center text-mono">
          {{ resistanceText }}
        </div>
        <div
          v-if="!hasResistanceValue"
          class="text-center text-caption text-error mt-2"
        >
          Not attached
        </div>
      </v-card-text>
    </v-card>
  </v-container>
</template>

<style scoped>
.measure-resistance {
  font-size: clamp(1.5rem, 5vw, 2.25rem);
  line-height: 1.2;
}
</style>
