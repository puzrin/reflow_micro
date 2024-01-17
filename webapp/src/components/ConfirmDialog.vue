<script setup lang="ts">
import { TransitionRoot, TransitionChild, Dialog, DialogPanel } from '@headlessui/vue'
import { ref } from 'vue';

type confirmResult = 'ok' | 'cancel' | 'dismiss'

let currentResolve: ((str: confirmResult) => void) | null = null
const isOpen = ref(false)

function run() {
  if (currentResolve) currentResolve('cancel')

  isOpen.value = true
  return new Promise(resolve => { currentResolve = resolve })
}

defineExpose({ run })

let closedAs: confirmResult = 'cancel'

function closeAs(result: confirmResult) {
  closedAs = result
  isOpen.value = false
}

// Resole promise only on transition end, to avoid problems with form validation.
// Form inputs must be "selectable" for proper reportValidity() effect.
function finish() {
  if (currentResolve) currentResolve(closedAs)
  currentResolve = null
}
</script>

<template>
  <TransitionRoot :show="isOpen" @after-leave="finish" appear as="template">
    <Dialog as="div" @close="closeAs('cancel')" class="relative z-10">
      <TransitionChild
        as="template"
        enter="duration-300 ease-out"
        enter-from="opacity-0"
        enter-to="opacity-100"
        leave="duration-200 ease-in"
        leave-from="opacity-100"
        leave-to="opacity-0"
      >
        <div class="fixed inset-0 bg-black/25" />
      </TransitionChild>

      <div class="fixed inset-0 overflow-y-auto">
        <div class="flex min-h-full items-center justify-center p-4 text-center">
          <TransitionChild
            as="template"
            enter="duration-300 ease-out"
            enter-from="opacity-0 scale-95"
            enter-to="opacity-100 scale-100"
            leave="duration-200 ease-in"
            leave-from="opacity-100 scale-100"
            leave-to="opacity-0 scale-95"
          >
            <DialogPanel
              class="w-full max-w-md transform overflow-hidden rounded-2xl bg-white p-6 text-left align-middle shadow-xl transition-all"
            >
              <slot :closeAs="closeAs"></slot>
            </DialogPanel>
          </TransitionChild>
        </div>
      </div>
    </Dialog>
  </TransitionRoot>
</template>
