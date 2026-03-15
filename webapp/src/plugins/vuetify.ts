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
    VAlert: {
      icon: false,
      rounded: 'lg',
    },
    VChip: {
      rounded: 'lg',
    },
    VSwitch: {
      inset: true,
    },
  },
})

export default vuetify
