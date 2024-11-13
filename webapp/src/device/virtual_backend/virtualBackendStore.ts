import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { HeaterConfig, ProfilesData } from '@/proto/generated/types';
import { DEFAULT_PROFILES_DATA_PB, DEFAULT_HEATER_CONFIG_PB } from '@/proto/generated/defaults'

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: ProfilesData.toJSON(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)) as string,
    rawHeaterConfig: HeaterConfig.toJSON(HeaterConfig.decode(DEFAULT_HEATER_CONFIG_PB)) as string
  })

  return { ...toRefs(state) }
}, { persist: true })
