<script setup lang="ts">
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink } from 'vue-router'
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { inject, ref } from 'vue'
import { type UseDraggableReturn, VueDraggable } from 'vue-draggable-plus'

import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import EditIcon from '@heroicons/vue/24/outline/PencilIcon'
import DeleteIcon from '@heroicons/vue/24/outline/XMarkIcon'
import DragIcon from '@heroicons/vue/24/outline/ArrowsUpDownIcon'

import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import ButtonDanger from '@/components/buttons/ButtonDanger.vue'
import ButtonNormalSquareSmall from '@/components/buttons/ButtonNormalSquareSmall.vue'

import ConfirmDialog from '@/components/ConfirmDialog.vue'
import { Device } from '@/device'

const profilesStore = useProfilesStore()
const device: Device = inject('device')!
const localSettingsStore = useLocalSettingsStore()
const el = ref<UseDraggableReturn>()

// On drag - move profile with store's method to keep reactivity working
function customUpdate(evt: any) {
  const { oldIndex, newIndex } = evt
  profilesStore.move(oldIndex, newIndex)
}

// Stuff for profile delete confirmation dialog

const deleteDlgRef = ref<InstanceType<typeof ConfirmDialog>>()
function deleteProfile(profileId: number) {
  deleteDlgRef.value?.run().then((result) => {
    if (result === 'ok') profilesStore.remove(profileId)
  })
}

// Stuff for reset profiles to default

const resetDlgRef = ref<InstanceType<typeof ConfirmDialog>>()
function resetProfiles() {
  resetDlgRef.value?.run().then((result) => {
    if (result === 'ok') {
      profilesStore.reset()
    }
  })
}

</script>

<template>
  <PageLayout>
    <template #toolbar>
      <RouterLink :to="{ name: 'home' }" class="mr-2 -ml-0.5">
        <BackIcon class="w-8 h-8" />
      </RouterLink>
      <span>Settings</span>
    </template>

    <h2 class="mb-4 text-lg font-semibold">Heating profiles</h2>

    <VueDraggable
      ref="el"
      v-model="profilesStore.items"
      :animation="150"
      handle=".handle"
      ghostClass="ghost"
      class="mb-4"
      :customUpdate="customUpdate"
    >
      <div
        v-for="profile in profilesStore.items"
        :key="profile.id"
        class="flex gap-x-2 items-center mb-3 rounded-lg p-2 border border-gray-30"
      >
        <div class="handle cursor-move">
          <DragIcon class="w-3 h-3" />
        </div>
        <div class="grow -ml-1 whitespace-nowrap text-ellipsis overflow-hidden">{{ profile.name }}</div>
        <RouterLink :to="{ name: 'profile', params: { id: profile.id } }">
          <ButtonNormalSquareSmall>
            <EditIcon class="w-4 h-4" />
          </ButtonNormalSquareSmall>
        </RouterLink>
        <ButtonNormalSquareSmall
          :disabled="profilesStore.items.length < 2"
          @click="deleteProfile(profile.id)"
        >
          <DeleteIcon class="w-4 h-4" />
        </ButtonNormalSquareSmall>
      </div>
    </VueDraggable>

    <div class="mb-4">
      <RouterLink :to="{ name: 'profile', params: { id: 0 } }" class="me-2">
        <ButtonNormal>Add</ButtonNormal>
      </RouterLink>
      <ButtonNormal @click="resetProfiles">Reset</ButtonNormal>
    </div>

    <!--<hr class="my-4">-->

    <h2 class="mb-2 text-lg font-semibold">General</h2>
    <!--<div class="flex items-center mb-4">
      <input
        checked
        id="sound-on"
        type="checkbox"
        value=""
        class="w-4 h-4 text-blue-600 bg-gray-100 border-gray-300 rounded focus:ring-blue-500"
      >
      <label for="sound-on" class="ms-2 text-gray-900">Enable sound</label>
    </div>-->

    <div class="flex items-center mb-4">
      <input
        id="debug-on"
        type="checkbox"
        v-model="localSettingsStore.showDebugInfo"
        class="w-4 h-4 text-blue-600 bg-gray-100 border-gray-300 rounded focus:ring-blue-500"
      >
      <label for="debug-on" class="ms-2 text-gray-900">Show debug info</label>
    </div>

    <div>
      <RouterLink :to="{ name: 'calibrate' }" class="me-2">
        <ButtonNormal>Calibrate</ButtonNormal>
      </RouterLink>
    </div>
  </PageLayout>

  <!-- Profiles reset confirmation dialog -->

  <ConfirmDialog ref="resetDlgRef" v-slot="{ closeAs }">
    <p class="mt-2">Confirm profiles reset? All your changes will be lost</p>
    <div class="mt-4">
      <ButtonDanger class="me-2" @click="closeAs('ok')">Yes</ButtonDanger>
      <ButtonNormal class="me-2" @click="closeAs('cancel')">Cancel</ButtonNormal>
    </div>
  </ConfirmDialog>

  <!-- Profile delete confirmation dialog -->

  <ConfirmDialog ref="deleteDlgRef" v-slot="{ closeAs }">
    <p class="mt-2">Confirm profile remove?</p>
    <div class="mt-4">
      <ButtonDanger class="me-2" @click="closeAs('ok')">Yes</ButtonDanger>
      <ButtonNormal class="me-2" @click="closeAs('cancel')">Cancel</ButtonNormal>
    </div>
  </ConfirmDialog>
</template>

<style scoped>
.ghost {
  opacity: 0.5;
  background: #c8ebfb;
}
</style>
