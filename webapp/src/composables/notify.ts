import { readonly, shallowRef } from 'vue'

export type NotifyOptions = {
  message: string
  color: 'success' | 'error' | 'info' | 'warning'
}

type NotifyState = NotifyOptions & {
  id: number
}

const state = shallowRef<NotifyState | null>(null)
let nextId = 0
let dismissTimeoutId: number | null = null

function clearDismissTimeout() {
  if (dismissTimeoutId !== null) {
    window.clearTimeout(dismissTimeoutId)
    dismissTimeoutId = null
  }
}

export function notify(options: NotifyOptions) {
  clearDismissTimeout()

  state.value = {
    id: ++nextId,
    message: options.message,
    color: options.color,
  }

  dismissTimeoutId = window.setTimeout(() => {
    dismiss()
  }, 2000)
}

function dismiss() {
  clearDismissTimeout()
  state.value = null
}

export function useNotifyHost() {
  return {
    state: readonly(state),
    dismiss,
  }
}
