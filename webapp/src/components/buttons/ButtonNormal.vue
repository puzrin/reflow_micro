<script setup lang="ts">
import { ref } from 'vue'

enum Effect { None, Success, Failure }

const currentEffect = ref<Effect>(Effect.None)
let timeoutId: number

const showSuccess = () => {
  if (timeoutId) clearTimeout(timeoutId)
  currentEffect.value = Effect.Success
  timeoutId = setTimeout(() => { currentEffect.value = Effect.None }, 500)
}

const showFailure = () => {
  if (timeoutId) clearTimeout(timeoutId)
  currentEffect.value = Effect.Failure
  timeoutId = setTimeout(() => { currentEffect.value = Effect.None }, 500)
}

defineExpose({ showSuccess, showFailure })
</script>

<template>
  <button
    type="button"
    class="text-gray-900 border border-gray-800 font-medium rounded-lg text-sm px-5 py-2.5 text-center
      hover:text-white hover:bg-gray-900
      focus:ring-4 focus:outline-none focus:ring-gray-300
      disabled:opacity-50 disabled:pointer-events-none"
    :class="{
      'bg-green-500 border-green-500 hover:bg-green-500 animate-pulse': currentEffect === Effect.Success,
      'bg-red-500 border-red-500 hover:bg-red-500 animate-shake': currentEffect === Effect.Failure
    }"
  >
    <slot/>
  </button>
</template>

<style scoped>
@keyframes shake {
  0%, 100% { transform: translateX(0); }
  25% { transform: translateX(-4px); }
  75% { transform: translateX(4px); }
}

.animate-shake {
  animation: shake 0.5s ease-in-out;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}

.animate-pulse {
  animation: pulse 0.5s ease-in-out infinite;
}
</style>