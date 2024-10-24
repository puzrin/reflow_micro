import { defineStore } from 'pinia'
import { ref } from 'vue';

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
    const rawProfilesData = ref<string>('')

  return { rawProfilesData }
}, { persist: true })
