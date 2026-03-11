<script setup lang="ts">
import { usePageShell } from '@/composables/appShell'

usePageShell(() => ({
  title: 'Not Found',
  nav: { kind: 'back', to: { name: 'home' } },
  pageMode: 'default',
}))
</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-alert type="error">
      Bad params. Go back to
      <RouterLink :to="{ name: 'home' }"><strong>home</strong></RouterLink>.
    </v-alert>
  </v-container>
</template>
