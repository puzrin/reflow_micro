<script setup lang="ts">
import { computed, onBeforeUnmount, ref, useAttrs } from 'vue'
import type { ComponentPublicInstance, PropType } from 'vue'
import { useRouter } from 'vue-router'

defineOptions({
  name: 'HoldToConfirmButton',
  inheritAttrs: false,
})

type ProgressMode = 'auto' | 'container' | 'subtle'

const props = defineProps({
  duration: {
    type: Number,
    default: 1300,
  },
  progressColor: {
    type: String,
    default: 'auto',
  },
  progressMode: {
    type: String as PropType<ProgressMode>,
    default: 'auto',
  },
})

const emit = defineEmits<{
  confirm: []
}>()

const attrs = useAttrs()
const router = useRouter()

const ROLLBACK_DURATION_MS = 200
const COMPLETE_SETTLE_MS = 140

const buttonRef = ref<ComponentPublicInstance | null>(null)
const activePointerId = ref<number | null>(null)
const activeKey = ref<'Enter' | ' ' | null>(null)
const progress = ref(0)
const isHolding = ref(false)
const isRollingBack = ref(false)
const isCompleted = ref(false)

let holdStartAt = 0
let holdFrameId: number | null = null
let rollbackTimeoutId: number | null = null
let completeTimeoutId: number | null = null

function isTruthyAttr(value: unknown) {
  return value === '' || !!value
}

const rawVariant = computed(() => {
  return typeof attrs.variant === 'string' ? attrs.variant : 'elevated'
})

const resolvedProgressMode = computed<Exclude<ProgressMode, 'auto'>>(() => {
  if (props.progressMode !== 'auto') return props.progressMode
  return rawVariant.value === 'text' || rawVariant.value === 'plain' ? 'subtle' : 'container'
})

const isBusy = computed(() => isTruthyAttr(attrs.loading) || isTruthyAttr(attrs.disabled))
const trackVisible = computed(() => isHolding.value || isRollingBack.value || progress.value > 0 || isCompleted.value)

const forwardedAttrs = computed(() => {
  return Object.fromEntries(
    Object.entries(attrs).filter(([key]) => !['onClick', 'target', 'rel', 'href', 'to', 'exact', 'replace'].includes(key))
  )
})

const buttonProps = computed(() => {
  const next = {
    ...forwardedAttrs.value,
  }

  if (!attrs.href && !attrs.to && attrs.type !== 'submit' && attrs.type !== 'reset') {
    return next
  }

  // Keep VBtn rendered as a real button so navigation/form submit can be delayed until hold-confirm finishes.
  return {
    ...next,
    tag: 'button',
  }
})

const buttonBindings = computed<Record<string, unknown>>(() => ({
  ...buttonProps.value,
}))

const cssVars = computed(() => {
  // `currentColor` follows the actual computed button color, which is more reliable than guessing a theme token.
  const progressColor = props.progressColor !== 'auto'
    ? `rgb(var(--v-theme-${props.progressColor}))`
    : 'currentColor'

  return {
    '--hold-progress-color': progressColor,
    '--hold-progress-size': `${progress.value * 100}%`,
    '--hold-progress-transition-duration': isRollingBack.value ? `${ROLLBACK_DURATION_MS}ms` : '0ms',
  }
})

function getButtonElement() {
  const instance = buttonRef.value as (ComponentPublicInstance & { $el?: Element }) | null
  return instance?.$el instanceof HTMLElement ? instance.$el : null
}

function isHoldKey(key: string): key is 'Enter' | ' ' | 'Spacebar' {
  return key === 'Enter' || key === ' ' || key === 'Spacebar'
}

function normalizeHoldKey(key: 'Enter' | ' ' | 'Spacebar') {
  return key === 'Spacebar' ? ' ' : key
}

function releasePointerCapture(pointerId = activePointerId.value) {
  const buttonEl = getButtonElement()
  if (pointerId === null || !buttonEl?.hasPointerCapture?.(pointerId)) return
  buttonEl.releasePointerCapture(pointerId)
}

function clearHoldFrame() {
  if (holdFrameId !== null) {
    cancelAnimationFrame(holdFrameId)
    holdFrameId = null
  }
}

function clearRollbackTimeout() {
  if (rollbackTimeoutId !== null) {
    window.clearTimeout(rollbackTimeoutId)
    rollbackTimeoutId = null
  }
}

function clearCompleteTimeout() {
  if (completeTimeoutId !== null) {
    window.clearTimeout(completeTimeoutId)
    completeTimeoutId = null
  }
}

function resetState() {
  clearHoldFrame()
  clearRollbackTimeout()
  clearCompleteTimeout()
  releasePointerCapture()

  activePointerId.value = null
  activeKey.value = null
  isHolding.value = false
  isRollingBack.value = false
  isCompleted.value = false
  progress.value = 0
}

function handleClickCapture(event: MouseEvent) {
  // VBtn activates on click by default; hold-to-confirm only exposes the completed hold path.
  event.preventDefault()
  event.stopPropagation()
  event.stopImmediatePropagation()
}

function handleContextMenu(event: MouseEvent) {
  event.preventDefault()
}

function startHoldFrame() {
  clearHoldFrame()

  const tick = () => {
    if (!isHolding.value) return

    const nextProgress = Math.min((performance.now() - holdStartAt) / props.duration, 1)
    progress.value = nextProgress

    if (nextProgress >= 1) {
      completeHold()
      return
    }

    holdFrameId = requestAnimationFrame(tick)
  }

  holdFrameId = requestAnimationFrame(tick)
}

function startRollback() {
  if (progress.value <= 0) {
    resetState()
    return
  }

  clearHoldFrame()
  clearRollbackTimeout()
  releasePointerCapture()

  activePointerId.value = null
  activeKey.value = null
  isHolding.value = false
  isRollingBack.value = true

  requestAnimationFrame(() => {
    progress.value = 0
  })

  rollbackTimeoutId = window.setTimeout(() => {
    isRollingBack.value = false
  }, ROLLBACK_DURATION_MS)
}

function finishAfterConfirm() {
  clearHoldFrame()
  clearRollbackTimeout()
  clearCompleteTimeout()
  releasePointerCapture()

  activePointerId.value = null
  activeKey.value = null
  isHolding.value = false
  isRollingBack.value = false
  isCompleted.value = true
  progress.value = 1

  completeTimeoutId = window.setTimeout(() => {
    isCompleted.value = false
    progress.value = 0
  }, COMPLETE_SETTLE_MS)
}

function navigateAfterConfirm() {
  if (attrs.to) {
    const navigation = isTruthyAttr(attrs.replace) ? router.replace : router.push
    void navigation(attrs.to as Parameters<typeof router.push>[0])
    return
  }

  if (typeof attrs.href === 'string') {
    const target = typeof attrs.target === 'string' ? attrs.target : undefined

    if (target && target !== '_self') {
      window.open(attrs.href, target)
      return
    }

    if (isTruthyAttr(attrs.replace)) {
      window.location.replace(attrs.href)
      return
    }

    window.location.assign(attrs.href)
    return
  }

  const buttonEl = getButtonElement()
  const type = typeof attrs.type === 'string' ? attrs.type : 'button'

  if (type === 'submit') {
    buttonEl?.closest('form')?.requestSubmit(buttonEl as HTMLButtonElement)
  } else if (type === 'reset') {
    buttonEl?.closest('form')?.reset()
  }
}

function completeHold() {
  if (!isHolding.value || isCompleted.value) return

  finishAfterConfirm()
  emit('confirm')
  navigateAfterConfirm()
}

function cancelHold() {
  if (isCompleted.value || (!isHolding.value && progress.value <= 0)) return
  startRollback()
}

function isPointerInside(event: PointerEvent) {
  const buttonEl = getButtonElement()
  if (!buttonEl) return false

  // Pointer capture keeps move events flowing after leaving the element, so use a live hit-test.
  const hitTarget = document.elementFromPoint(event.clientX, event.clientY)
  return !!hitTarget && (hitTarget === buttonEl || buttonEl.contains(hitTarget))
}

function handlePointerDown(event: PointerEvent) {
  if (isBusy.value || isHolding.value || activePointerId.value !== null || activeKey.value !== null || isCompleted.value) return
  if (event.pointerType === 'mouse' && event.button !== 0) return

  clearRollbackTimeout()
  clearCompleteTimeout()

  activePointerId.value = event.pointerId
  isHolding.value = true
  isRollingBack.value = false
  isCompleted.value = false
  progress.value = 0
  holdStartAt = performance.now()

  event.preventDefault()
  getButtonElement()?.setPointerCapture?.(event.pointerId)
  startHoldFrame()
}

function handlePointerUp(event: PointerEvent) {
  if (event.pointerId !== activePointerId.value || isCompleted.value) return
  cancelHold()
}

function handlePointerCancel(event: PointerEvent) {
  if (event.pointerId !== activePointerId.value || isCompleted.value) return
  cancelHold()
}

function handleLostPointerCapture(event: PointerEvent) {
  if (event.pointerId !== activePointerId.value || isCompleted.value) return
  cancelHold()
}

function handlePointerMove(event: PointerEvent) {
  if (event.pointerId !== activePointerId.value || isCompleted.value) return
  if (!isPointerInside(event)) cancelHold()
}

function handleKeyDown(event: KeyboardEvent) {
  if (!isHoldKey(event.key)) return
  if (event.metaKey || event.ctrlKey || event.altKey) return

  event.preventDefault()

  if (event.repeat) return
  if (isBusy.value || isHolding.value || activePointerId.value !== null || activeKey.value !== null || isCompleted.value) return

  clearRollbackTimeout()
  clearCompleteTimeout()

  activeKey.value = normalizeHoldKey(event.key)
  isHolding.value = true
  isRollingBack.value = false
  isCompleted.value = false
  progress.value = 0
  holdStartAt = performance.now()

  startHoldFrame()
}

function handleKeyUp(event: KeyboardEvent) {
  if (!isHoldKey(event.key)) return
  if (normalizeHoldKey(event.key) !== activeKey.value || isCompleted.value) return

  event.preventDefault()
  cancelHold()
}

function handleBlur() {
  if (activeKey.value !== null && !isCompleted.value) {
    cancelHold()
  }
}

onBeforeUnmount(() => {
  resetState()
})
</script>

<template>
  <v-btn
    ref="buttonRef"
    v-bind="buttonBindings"
    :class="[
      'hold-to-confirm-button',
      `hold-to-confirm-button--mode-${resolvedProgressMode}`,
      {
        'hold-to-confirm-button--holding': isHolding,
        'hold-to-confirm-button--rolling-back': isRollingBack,
        'hold-to-confirm-button--track-visible': trackVisible,
        'hold-to-confirm-button--completed': isCompleted,
      },
    ]"
    :style="cssVars"
    @click.capture="handleClickCapture"
    @contextmenu="handleContextMenu"
    @blur="handleBlur"
    @keydown="handleKeyDown"
    @keyup="handleKeyUp"
    @lostpointercapture="handleLostPointerCapture"
    @pointercancel="handlePointerCancel"
    @pointerdown="handlePointerDown"
    @pointermove="handlePointerMove"
    @pointerup="handlePointerUp"
  >
    <template v-if="$slots.prepend" #prepend>
      <slot name="prepend" />
    </template>

    <slot />

    <template v-if="$slots.append" #append>
      <slot name="append" />
    </template>

    <template v-if="$slots.loader" #loader>
      <slot name="loader" />
    </template>
  </v-btn>
</template>

<style scoped>
.hold-to-confirm-button {
  --hold-track-strength: 0%;
  --hold-fill-strength: 20%;
  --hold-track-color: transparent;
  --hold-fill-color: transparent;

  /* Use background layers for progress so we don't fight VBtn's own overlay/focus-ring pseudo-elements. */
  background-image:
    linear-gradient(
      to right,
      var(--hold-fill-color),
      var(--hold-fill-color)
    ),
    linear-gradient(
      to right,
      var(--hold-track-color),
      var(--hold-track-color)
    );
  background-position: left top, left top;
  background-repeat: no-repeat;
  background-size: var(--hold-progress-size) 100%, 100% 100%;
  overflow: hidden;
  touch-action: none;
  transition-duration: 0.28s, 0.28s, 0.28s, 0.28s, var(--hold-progress-transition-duration);
  transition-property: box-shadow, transform, opacity, background, background-size;
  transition-timing-function:
    cubic-bezier(0.4, 0, 0.2, 1),
    cubic-bezier(0.4, 0, 0.2, 1),
    cubic-bezier(0.4, 0, 0.2, 1),
    cubic-bezier(0.4, 0, 0.2, 1),
    cubic-bezier(0.2, 0, 0, 1);
  -webkit-touch-callout: none;
}

.hold-to-confirm-button--track-visible {
  --hold-track-color: color-mix(in srgb, var(--hold-progress-color) var(--hold-track-strength), transparent);
  --hold-fill-color: color-mix(in srgb, var(--hold-progress-color) var(--hold-fill-strength), transparent);
}

.hold-to-confirm-button--mode-container {
  --hold-fill-strength: 20%;
}

.hold-to-confirm-button--mode-subtle {
  --hold-track-strength: 8%;
  --hold-fill-strength: 16%;
}
</style>
