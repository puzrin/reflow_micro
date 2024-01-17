import { defineStore } from 'pinia'
import { ref } from 'vue';

export const useLocalSettingsStore = defineStore('localSettings', () => {
  const showDebugInfo = ref(true);

  return { showDebugInfo }
}, { persist: true })
