import { presetIcons } from '@unocss/preset-icons'
import { aliases as msAliases } from 'vuetify/iconsets/ms'

const appIcons = [
  'i-material-symbols:arrow-back',
  'i-material-symbols:more-vert',
  'i-material-symbols:drag-indicator',
  'i-material-symbols:add',
  'i-material-symbols:delete-outline',
  'i-material-symbols:check',
]

export default {
  presets: [
    presetIcons(),
  ],
  safelist: [
    ...Object.values(msAliases),
    ...appIcons,
  ],
}
