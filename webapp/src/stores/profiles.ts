import type { Profile, ProfilesStoreData } from '@/device/heater_config'
import { defineStore } from 'pinia'
import { computed, reactive, ref, toRaw } from 'vue';
import { defaultProfilesStoreData } from "@/device/heater_config"

export const useProfilesStore = defineStore('profiles', () => {
  const items = reactive([] as Profile[]);
  const selectedId = ref<number>(-1);

  const selected = computed(() => find(selectedId.value) || null)

  function exists(id: number) { return !!items.find(p => p.id === id) }

  function find(id: number) { return items.find(p => p.id === id) }

  function add(profile: Profile): number {
    // Deep clone to keep the original object immutable
    const p = structuredClone(profile)
    // Allocate a new ID if needed
    if (!p.id) p.id = Math.max(...items.map(item => item.id), 0) + 1

    if (exists(p.id)) {
      // Update existing profile
      const idx = items.findIndex(p => p.id === profile.id)
      items[idx] = p
    } else {
      items.push(p)
      if (selectedId.value < 0) selectedId.value = p.id
    }

    return p.id
  }

  function remove(id: number) {
    // Make sure item exists
    const idx = items.findIndex(p => p.id === id)
    if (idx < 0) return

    items.splice(idx, 1)

    // Reselect profile if needed
    if (selectedId.value === id) selectedId.value = items.length ? items[0].id : -1
  }

  function select(id: number) {
    if (exists(id)) selectedId.value = id
  }

  function move(oldIndex: number, newIndex: number) {
    const item = items.splice(oldIndex, 1)[0]
    items.splice(newIndex, 0, item)
  }

  function toRawObj(): ProfilesStoreData {
    return {
      selectedId: selectedId.value,
      items: toRaw(items)
    }
  }

  function fromRawObj(data: ProfilesStoreData) {
    items.splice(0, items.length, ...data.items)
    selectedId.value = data.selectedId
  }

  function reset() {
    fromRawObj(defaultProfilesStoreData)
  }

  return { items, selectedId, selected, exists, find, add, remove, select, move, reset, toRawObj, fromRawObj }
})
