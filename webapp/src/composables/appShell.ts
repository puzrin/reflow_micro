import { onScopeDispose, reactive, watchEffect } from 'vue'
import type { RouteLocationRaw } from 'vue-router'

export type AppShellNav =
  | { kind: 'menu' }
  | { kind: 'back'; to: RouteLocationRaw }
  | { kind: 'none' }

export type AppShellPageMode = 'default' | 'fit-viewport'

export type AppShellState = {
  title: string
  nav: AppShellNav
  pageMode: AppShellPageMode
}

const defaultState = (): AppShellState => ({
  title: '',
  nav: { kind: 'none' },
  pageMode: 'default',
})

const state = reactive<AppShellState>(defaultState())
let activeToken: symbol | null = null

function assignState(next: AppShellState) {
  state.title = next.title
  state.nav = next.nav
  state.pageMode = next.pageMode
}

function resetState() {
  assignState(defaultState())
}

export function useAppShellState(): AppShellState {
  return state
}

export function usePageShell(getState: () => AppShellState) {
  const token = Symbol('page-shell')

  watchEffect(() => {
    activeToken = token
    assignState(getState())
  })

  onScopeDispose(() => {
    if (activeToken === token) {
      activeToken = null
      resetState()
    }
  })
}
