import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { type backendIdType } from '@/device'
import { BleBackend } from '@/device/ble_backend';

export const useLocalSettingsStore = defineStore('localSettings', () => {
  const state = reactive({
    showDebugInfo: true,
    profileEditorShowPreview: false,
    backend: BleBackend.id as backendIdType,
    bleName: 'Reflow Table'
  })
  return { ...toRefs(state) }
}, { persist: true })
