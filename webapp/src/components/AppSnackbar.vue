<script setup lang="ts">
import { useNotifyHost } from '@/composables/notify'

const { state, dismiss } = useNotifyHost()

function handleModelValue(next: boolean) {
  if (!next) dismiss()
}
</script>

<template>
  <v-snackbar
    :model-value="Boolean(state)"
    :color="state?.color"
    :timeout="-1"
    @update:model-value="handleModelValue"
  >
    {{ state?.message ?? '' }}
  </v-snackbar>
</template>
