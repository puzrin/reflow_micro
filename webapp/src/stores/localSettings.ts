import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';

export const useLocalSettingsStore = defineStore('localSettings', () => {
  const state = reactive({
    showDebugInfo: true,
    profileEditorShowPreview: false,
    demoMode: false
  })
  return { ...toRefs(state) }
}, { persist: true })
