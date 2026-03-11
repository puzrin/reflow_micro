<script setup lang="ts">
import { useNotifyHost } from '@/composables/notify'

const { state, dismiss } = useNotifyHost()

function handleModelValue(next: boolean) {
  if (!next) dismiss()
}
</script>

<template>
  <v-snackbar
    v-if="state"
    :key="state.id"
    :model-value="true"
    :color="state.color"
    @update:model-value="handleModelValue"
  >
    {{ state.message }}
  </v-snackbar>
</template>
