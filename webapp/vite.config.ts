import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import Vuetify from 'vite-plugin-vuetify'
import { VitePWA } from 'vite-plugin-pwa'
import UnoCSS from 'unocss/vite'

import gitRemoteOriginUrl from 'git-remote-origin-url'

// https://vitejs.dev/config/
export default defineConfig(async () => {
  let base = '/'
  let repoUrl = ''

  try {
    const origin = await gitRemoteOriginUrl()
    // Possible formats:
    // - https://github.com/user/repo
    // - git@github.com:user/repo.git
    const match = origin.match(/git@github[.]com:([^/]+)\/([^/]+)\.git$/) ||
      origin.match(/https:\/\/github[.]com\/([^/]+)\/([^/]+)$/)

    if (match) {
      base = `/${match[2]}/`
      repoUrl = `https://github.com/${match[1]}/${match[2]}`
    }
  } catch {/* ignore */}

  return {
    base: base,
    define: {
      __REPO_URL__: JSON.stringify(repoUrl),
    },
    optimizeDeps: {
      // MS icons are done via UnoCSS. This workaround makes it work properly
      // on dev server. Production build is ok.
      exclude: ['vuetify/iconsets/ms'],
    },
    plugins: [
      UnoCSS(),
      vue(),
      Vuetify(),
      VitePWA({
        registerType: 'autoUpdate',
        pwaAssets: {},
        manifest: {
          name: 'Reflow Table',
          short_name: 'Reflow',
          description: 'Control and monitor the reflow table.',
          start_url: '.',
          display: 'standalone',
          scope: '.',
          theme_color: '#111827',
          background_color: '#f8fafc',
        }
      }),
    ],
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url))
      }
    }
  }
})
