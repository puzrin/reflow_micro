import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { type AdrcConfig, defaultAdrcConfig } from '../adrc_config';

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: '',
    sensor_calibration_status: [false, false] as [boolean, boolean],
    adrc_config: structuredClone(defaultAdrcConfig)
  })

  return { ...toRefs(state) }
}, { persist: true })
