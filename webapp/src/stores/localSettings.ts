import { defineStore } from 'pinia'
import { toRefs, reactive } from 'vue';
import { type backendIdType } from '@/device'
import { BleBackend } from '@/device/ble_backend';

export const THEME_MODES = ['auto', 'light', 'dark'] as const
export type ThemeMode = (typeof THEME_MODES)[number]

export function normalizeThemeMode(value: unknown): ThemeMode {
  return typeof value === 'string' && THEME_MODES.includes(value as ThemeMode)
    ? value as ThemeMode
    : 'auto'
}

function sanitizeThemeMode(target: { themeMode: unknown }) {
  target.themeMode = normalizeThemeMode(target.themeMode)
}

export const useLocalSettingsStore = defineStore('localSettings', () => {
  const state = reactive({
    showDebugInfo: true,
    profileEditorShowPreview: false,
    backend: BleBackend.id as backendIdType,
    bleName: 'Reflow Table',
    themeMode: 'auto' as ThemeMode,
  })

  return { ...toRefs(state) }
}, {
  persist: {
    afterHydrate: ({ store }) => sanitizeThemeMode(store as unknown as { themeMode: unknown }),
  },
})
