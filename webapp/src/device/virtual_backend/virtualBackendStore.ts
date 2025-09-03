import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { HeadParams, ProfilesData } from '@/proto/generated/types';
import { DEFAULT_PROFILES_DATA_PB, DEFAULT_HEAD_PARAMS_PB } from '@/proto/generated/defaults'

export const useVirtualBackendStore = defineStore('virtualBackendStore', () => {
  const state = reactive({
    rawProfilesData: ProfilesData.toJSON(ProfilesData.decode(DEFAULT_PROFILES_DATA_PB)),
    rawHeadParams: HeadParams.toJSON(HeadParams.decode(DEFAULT_HEAD_PARAMS_PB))
  })

  return { ...toRefs(state) }
}, { persist: true })
