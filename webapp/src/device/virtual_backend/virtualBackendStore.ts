import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { defaultAdrcConfig } from '../adrc_config';
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/profiles_data_pb'

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: Array.from(DEFAULT_PROFILES_DATA_PB) as number[],
    sensor_calibration_status: [false, false] as [boolean, boolean],
    adrc_config: structuredClone(defaultAdrcConfig)
  })

  return { ...toRefs(state) }
}, { persist: true })
