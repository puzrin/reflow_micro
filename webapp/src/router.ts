import { createRouter, createWebHashHistory } from 'vue-router'
import HomeView from './views/HomeView.vue'
import SettingsView from './views/SettingsView.vue'
import CalibrateView from './views/CalibrateView.vue'
import ProfileView from './views/ProfileView.vue'
import NotFound from './views/NotFound.vue'

const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: HomeView
    },
    {
      path: '/settings',
      name: 'settings',
      component: SettingsView
    },
    {
      path: '/calibrate',
      name: 'calibrate',
      component: CalibrateView
    },
    {
      path: '/profile/:id(0|[1-9]\\d*)',
      name: 'profile',
      component: ProfileView,
      props: route => ({ ...route.params, id: Number(route.params.id) })
    },
    {
      path: '/:pathMatch(.*)*',
      component: NotFound
    }
  ]
})

export default router
