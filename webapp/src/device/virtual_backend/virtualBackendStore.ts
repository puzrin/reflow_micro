import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { DEFAULT_PROFILES_DATA_PB } from '@/proto/generated/profiles_data_pb'
import { HeaterConfigs, ProfilesData } from '@/proto/generated/types';

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: ProfilesData.toJSON(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)) as string,
    rawHeaterConfigs: HeaterConfigs.toJSON(HeaterConfigs.create()) as string
  })

  return { ...toRefs(state) }
}, { persist: true })
