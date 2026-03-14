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
      Invalid page parameters. Return to the
      <RouterLink :to="{ name: 'home' }"><strong>home</strong></RouterLink> page.
    </v-alert>
  </v-container>
</template>
