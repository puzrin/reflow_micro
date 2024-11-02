import { defineStore } from 'pinia'
import { ref } from 'vue';
import { type AdrcConfig, defaultAdrcConfig } from '../adrc_config';

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const rawProfilesData = ref<string>('')
  const sensor_calibration_status = ref<[boolean, boolean]>([false, false])
  const adrc_config = ref<AdrcConfig>(JSON.parse(JSON.stringify(defaultAdrcConfig)))

  return { rawProfilesData, sensor_calibration_status, adrc_config }
}, { persist: true })
