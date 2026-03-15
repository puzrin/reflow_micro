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
})

export default vuetify
