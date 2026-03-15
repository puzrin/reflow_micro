import 'vuetify/styles'

import { createVuetify } from 'vuetify'
import { md3 } from 'vuetify/blueprints'
import { aliases as msAliases, ms } from 'vuetify/iconsets/ms'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'

const vuetify = createVuetify({
  blueprint: md3,
  components,
  directives,
  icons: {
    defaultSet: 'ms',
    aliases: msAliases,
    sets: {
      ms,
    },
  },
  defaults: {
    VAppBar: {
      flat: false, // Not MD3 but looks better
    },
    VAlert: {
      icon: false,    // Custom, reduce the noise
      rounded: 'lg',  // MD3 fix
    },
    VChip: {
      rounded: 'lg',  // MD3 fix
    },
    VSwitch: {
      inset: true,    // MD3 fix
    },
  },
  theme: {
    themes: {
      // MD3 dark theme fix
      dark: {
        colors: {
          background: '#121212',
          surface: '#1E1E1E',
          'surface-bright': '#3B3B3B',
          'surface-variant': '#49454F',
          'on-surface': '#E6E0E9',
          'on-surface-variant': '#CAC4D0',
          primary: '#D0BCFF',
          'on-primary': '#381E72',
          secondary: '#CCC2DC',
          'on-secondary': '#332D41',
          error: '#F2B8B5',
          'on-error': '#601410',
        },
      },
    },
  }
})

export default vuetify
