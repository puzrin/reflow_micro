<script setup lang="ts">
import { RouterView } from 'vue-router'
</script>

<template>
  <RouterView :key="$route.fullPath" />
</template>
