<script setup lang="ts">
import { computed } from 'vue'
import { useConfirmHost } from '@/composables/confirm'

const { state, cancel, choose } = useConfirmHost()

const isOpen = computed(() => state.value !== null)

function handleModelValue(next: boolean) {
  if (!next) cancel()
}
</script>

<template>
  <v-dialog
    :model-value="isOpen"
    max-width="30rem"
    @click:outside="cancel"
    @update:model-value="handleModelValue"
  >
    <v-card v-if="state">
      <v-card-item :title="state.title" />
      <v-card-text v-if="state.description">
        {{ state.description }}
      </v-card-text>
      <v-card-actions class="justify-end">
        <v-btn
          v-for="action in state.actions"
          :key="action.key"
          :color="action.color"
          :variant="action.variant ?? (action.color ? 'elevated' : 'text')"
          @click="choose(action.key)"
        >
          {{ action.label }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
