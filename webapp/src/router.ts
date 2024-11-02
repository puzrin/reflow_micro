import { createRouter, createWebHashHistory } from 'vue-router'
import HomeView from './views/HomeView.vue'
import SettingsView from './views/SettingsView.vue'
import CalibrateSensorView from './views/CalibrateSensorView.vue'
import CalibrateAdrcView from './views/CalibrateAdrcView.vue'
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
      path: '/calibrate_sensor',
      name: 'calibrate_sensor',
      component: CalibrateSensorView
    },
    {
      path: '/calibrate_adrc',
      name: 'calibrate_adrc',
      component: CalibrateAdrcView
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
