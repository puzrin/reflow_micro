<script setup lang="ts">
import DeviceConnectionInfo from '@/components/DeviceConnectionInfo.vue'
</script>

<template>
  <div class="flex relative flex-col min-h-screen w-full max-w-screen-md mx-auto bg-white">
    <div class="flex items-center px-4 h-16 shrink-0 text-2xl text-slate-400">
      <slot name="toolbar"></slot>
    </div>
    <div class="flex-1 flex flex-col px-4 pb-4 text-slate-700 grow">
      <slot></slot>
    </div>

    <DeviceConnectionInfo />
  </div>
</template>
