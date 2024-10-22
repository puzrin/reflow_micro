import { defineStore } from 'pinia'
import { ref } from 'vue';

export const useLocalSettingsStore = defineStore('localSettings', () => {
  const showDebugInfo = ref(true);
  const profileEditorShowPreview = ref(false);
  const demoMode = ref(false);

  return { showDebugInfo, profileEditorShowPreview, demoMode }
}, { persist: true })
