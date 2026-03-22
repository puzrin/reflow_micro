import './main.css'
import 'virtual:uno.css'
import '@fontsource/roboto/latin-400.css'
import '@fontsource/roboto/latin-500.css'
import '@fontsource/roboto/latin-700.css'

import { createApp, watch } from 'vue'
import { createPinia } from 'pinia'
import piniaPluginPersistedstate from 'pinia-plugin-persistedstate'
import { registerSW } from 'virtual:pwa-register'

import App from './App.vue'
import router from './router'
import device from './device'
import vuetify from './plugins/vuetify'
import { normalizeThemeMode, useLocalSettingsStore } from './stores/localSettings'

const app = createApp(App)

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

app.use(pinia)

const localSettingsStore = useLocalSettingsStore(pinia)

function applyThemeMode(mode: unknown) {
  const normalizedMode = normalizeThemeMode(mode)
  if (localSettingsStore.themeMode !== normalizedMode) {
    localSettingsStore.themeMode = normalizedMode
  }

  vuetify.theme.change(normalizedMode === 'auto' ? 'system' : normalizedMode)
}

applyThemeMode(localSettingsStore.themeMode)
watch(() => localSettingsStore.themeMode, applyThemeMode)

app.use(router)
app.use(device)
app.use(vuetify)

app.mount('#app')

const updateSW = registerSW({
  immediate: true,
  onNeedRefresh() {
    updateSW()
  },
  onOfflineReady() {
    console.info('App ready for offline use.')
  },
})
