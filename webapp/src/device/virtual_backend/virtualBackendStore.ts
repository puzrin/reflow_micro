import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { AdrcParams, SensorParams, ProfilesData } from '@/proto/generated/types';
import { DEFAULT_PROFILES_DATA_PB, DEFAULT_ADRC_PARAMS_PB, DEFAULT_SENSOR_PARAMS_PB } from '@/proto/generated/defaults'

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: ProfilesData.toJSON(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)),
    rawAdrcParams: AdrcParams.toJSON(AdrcParams.decode(DEFAULT_ADRC_PARAMS_PB)),
    rawSensorParams: SensorParams.toJSON(SensorParams.decode(DEFAULT_SENSOR_PARAMS_PB))
  })

  return { ...toRefs(state) }
}, { persist: true })
