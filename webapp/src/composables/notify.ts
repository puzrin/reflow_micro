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

export function notify(options: NotifyOptions) {
  state.value = {
    id: ++nextId,
    message: options.message,
    color: options.color,
  }
}

function dismiss() {
  state.value = null
}

export function useNotifyHost() {
  return {
    state: readonly(state),
    dismiss,
  }
}
