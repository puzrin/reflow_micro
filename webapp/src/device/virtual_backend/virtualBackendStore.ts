import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { HeaterParams, ProfilesData } from '@/proto/generated/types';
import { DEFAULT_PROFILES_DATA_PB, DEFAULT_HEATER_PARAMS_PB } from '@/proto/generated/defaults'

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: ProfilesData.toJSON(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)) as string,
    rawHeaterParams: HeaterParams.toJSON(HeaterParams.decode(DEFAULT_HEATER_PARAMS_PB)) as string
  })

  return { ...toRefs(state) }
}, { persist: true })
