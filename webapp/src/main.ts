import './main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import piniaPluginPersistedstate from 'pinia-plugin-persistedstate'
import { registerSW } from 'virtual:pwa-register'

import App from './App.vue'
import router from './router'
import device from './device'

const app = createApp(App)

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

app.use(pinia)
app.use(router)
app.use(device)

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
