import './main.css'
import '@fontsource/roboto/300.css'
import '@fontsource/roboto/400.css'
import '@fontsource/roboto/500.css'
import '@fontsource/roboto/700.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import piniaPluginPersistedstate from 'pinia-plugin-persistedstate'
import { registerSW } from 'virtual:pwa-register'

import App from './App.vue'
import router from './router'
import device from './device'
import vuetify from './plugins/vuetify'

const app = createApp(App)

const pinia = createPinia()
pinia.use(piniaPluginPersistedstate)

app.use(pinia)
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
