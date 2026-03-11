import { readonly, shallowRef } from 'vue'

export type ConfirmAction = {
  key: string
  label: string
  color?: 'primary' | 'error'
  variant?: 'elevated' | 'text' | 'flat'
}

export type ConfirmOptions = {
  title: string
  description?: string
  actions: ConfirmAction[]
  cancelKey?: string
}

type ConfirmState = {
  title: string
  description?: string
  actions: ConfirmAction[]
  cancelKey: string
}

const state = shallowRef<ConfirmState | null>(null)
let currentResolve: ((result: string) => void) | null = null

function finish(result: string) {
  const resolve = currentResolve
  currentResolve = null
  state.value = null
  resolve?.(result)
}

function cancel() {
  if (!state.value) return
  finish(state.value.cancelKey)
}

function choose(actionKey: string) {
  finish(actionKey)
}

export function confirm(options: ConfirmOptions): Promise<string> {
  cancel()

  state.value = {
    title: options.title,
    description: options.description,
    actions: options.actions.map(action => ({ ...action })),
    cancelKey: options.cancelKey ?? 'cancel',
  }

  return new Promise(resolve => {
    currentResolve = resolve
  })
}

export function useConfirmHost() {
  return {
    state: readonly(state),
    cancel,
    choose,
  }
}
