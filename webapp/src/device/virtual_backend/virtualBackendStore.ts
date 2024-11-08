import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/profiles_data_pb'
import { DEFAULT_ADRC_CONFIG_PB } from '@/proto/generated/adrc_config_pb';
import { AdrcConfig } from '@/proto/generated/types';

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: Array.from(DEFAULT_PROFILES_DATA_PB) as number[],
    sensor_calibration_status: [false, false] as [boolean, boolean],
    adrc_config: AdrcConfig.decode(DEFAULT_ADRC_CONFIG_PB)
  })

  return { ...toRefs(state) }
}, { persist: true })
