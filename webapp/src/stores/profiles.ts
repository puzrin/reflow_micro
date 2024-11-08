import { defineStore } from 'pinia'
import { computed, reactive, toRefs } from 'vue';
import { Profile, ProfilesData } from '@/proto/generated/types'

export const useProfilesStore = defineStore('profiles', () => {
  const state = reactive<ProfilesData>({
    items: [],
    selectedId: -1
  })

  const selected = computed(() => find(state.selectedId) || null)

  function exists(id: number) { return !!state.items.find(p => p.id === id) }

  function find(id: number) { return state.items.find(p => p.id === id) }

  function add(profile: Profile): number {
    // Deep clone to keep the original object immutable
    const p = structuredClone(profile)
    // Allocate a new ID if needed
    if (!p.id) p.id = Math.max(...state.items.map(item => item.id), 0) + 1

    if (exists(p.id)) {
      // Update existing profile
      const idx = state.items.findIndex(p => p.id === profile.id)
      state.items[idx] = p
    } else {
      state.items.push(p)
      if (state.selectedId < 0) state.selectedId = p.id
    }

    return p.id
  }

  function remove(id: number) {
    // Make sure item exists
    const idx = state.items.findIndex(p => p.id === id)
    if (idx < 0) return

    state.items.splice(idx, 1)

    // Reselect profile if needed
    if (state.selectedId === id) state.selectedId = state.items.length ? state.items[0].id : -1
  }

  function select(id: number) {
    if (exists(id)) state.selectedId = id
  }

  function move(oldIndex: number, newIndex: number) {
    const item = state.items.splice(oldIndex, 1)[0]
    state.items.splice(newIndex, 0, item)
  }

  return { ...toRefs(state), selected, exists, find, add, remove, select, move }
})
