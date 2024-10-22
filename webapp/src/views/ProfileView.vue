<script setup lang="ts">
import PageLayout from '@/components/PageLayout.vue'
import { RouterLink, onBeforeRouteLeave } from 'vue-router'
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { reactive, ref, toRaw, watch } from 'vue'
import { startTemperature, limits, createDummyProfile, type Profile } from '@/device/heater_config'

import ButtonNormal from '@/components/buttons/ButtonNormal.vue'
import ButtonDanger from '@/components/buttons/ButtonDanger.vue'
import ButtonNormalSquareSmall from '@/components/buttons/ButtonNormalSquareSmall.vue'

import BackIcon from '@heroicons/vue/24/outline/ArrowLeftIcon'
import AddIcon from '@heroicons/vue/24/outline/PlusIcon'
import DeleteIcon from '@heroicons/vue/24/outline/XMarkIcon'

import ConfirmDialog from '@/components/ConfirmDialog.vue'
import ReflowChart from '@/components/ReflowChart.vue'

const props = defineProps<{ id: number }>()
const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()

// Load profile object from store or create new one
const srcProfile: Profile = profilesStore.exists(props.id) ?
  toRaw(profilesStore.find(props.id))! : createDummyProfile()

// Create reactive clone to accumulate and track changes
const profile = reactive(structuredClone(srcProfile)!)
const isProfileEdited = ref(false)

watch(profile, () => isProfileEdited.value = true, { deep: true })

const formRef = ref<HTMLFormElement>()

function saveForm() {
  // Save profile to store & auto-update id to avoid dupes on next save
  profile.id = profilesStore.add(toRaw(profile))
  isProfileEdited.value = false
}

const exitDlgRef = ref<InstanceType<typeof ConfirmDialog>>()

onBeforeRouteLeave(async () => {
  if (!isProfileEdited.value) return true

  const result = await exitDlgRef.value?.run()
  if (result === 'cancel') return false
  if (result === 'dismiss') return true

  if (formRef.value?.checkValidity()) {
    saveForm()
    return true
  }

  formRef.value?.reportValidity()
  return false
})

function duplicateSegment(segmentIdx: number) {
  const newSegment = structuredClone(toRaw(profile).segments[segmentIdx])
  profile.segments.splice(segmentIdx, 0, newSegment)
}

function heatingSpeed(segmentIdx: number) {
  const start = segmentIdx === 0 ? startTemperature : (profile.segments[segmentIdx - 1].target || 0)
  const segment = profile.segments[segmentIdx]
  const speed = (segment.target - start) / (segment.duration || 0)

  if (Math.abs(speed) < 0.0001) return `Keep`
  if (Math.abs(speed) > 0.1) return `${speed.toFixed(1)}°C/sec`
  return `${(speed * 60).toFixed(1)}°C/min`
}

// Cleanup & recast for numeric inputs
const str2int = (str: string) => parseInt(str.replace(/[^0-9]/g, '')) || 0
</script>

<template>
  <PageLayout>
    <template #toolbar>
      <RouterLink :to="{ name: 'settings' }" class="mr-2 -ml-0.5">
        <BackIcon class="w-8 h-8" />
      </RouterLink>
      <span v-if="profile.id">Edit profile</span>
      <span v-else>New profile</span>
    </template>

    <form ref="formRef" @submit.prevent="saveForm">
      <div class="mb-4">
        <input
          type="text"
          required
          v-model.trim="profile.name"
          placeholder="Name"
          class="w-full"
          :min="limits.nameMin"
          :max="limits.nameMax"
        />
      </div>

      <div class="mb-4">
        <div>
          <ButtonNormal @click="localSettingsStore.profileEditorShowPreview = !localSettingsStore.profileEditorShowPreview" class="w-full">
            Preview
          </ButtonNormal>
        </div>
        <Transition name="bounce">
          <div v-if="localSettingsStore.profileEditorShowPreview" class="mt-4 relative rounded-md bg-slate-100 h-[300px]">
            <div class="absolute top-0 left-0 right-0 bottom-0">
              <ReflowChart id="profile-edit-chart" :profile="profile" />
            </div>
          </div>
        </Transition>
      </div>

      <h2 class="mb-4 mt-4 text-lg font-semibold">Stages</h2>

      <div v-for="(segment, index) in profile.segments" :key="index">
        <div class="mb-1 text-sm">
          <span>{{ heatingSpeed(index) }}</span>,
          <span> {{ segment.duration < 600 ? segment.duration + 'sec' : (segment.duration/60).toFixed() + 'min' }}</span>
        </div>
        <div class="flex gap-3 mb-4">
          <div class="w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              :value="segment.target"
              @input="segment.target = str2int(($event?.target as HTMLInputElement).value)"
              class="w-full"
              :min="limits.targetMin"
              :max="limits.targetMax"
            />
            <div class="text-xs text-slate-400 mt-0.5">Target °C</div>
          </div>
          <div class="w-full">
            <input
              type="number"
              required
              inputmode="numeric"
              :value="segment.duration"
              @input="segment.duration = str2int(($event?.target as HTMLInputElement).value)"
              class="w-full"
              :min="limits.durationMin"
              :max="limits.durationMax"
            />
            <div class="text-xs text-slate-400 mt-0.5">Duration, sec</div>
          </div>
          <div>
            <ButtonNormalSquareSmall @click="duplicateSegment(index)">
              <AddIcon class="w-6 h-6" />
            </ButtonNormalSquareSmall>
          </div>
          <div>
            <ButtonNormalSquareSmall
              @click="profile.segments.splice(index, 1)"
              :disabled="profile.segments.length < 2"
            >
              <DeleteIcon class="w-6 h-6" />
            </ButtonNormalSquareSmall>
          </div>
        </div>
      </div>

      <div class="mt-6">
        <ButtonNormal type="submit">Save</ButtonNormal>
      </div>
    </form>
  </PageLayout>

  <ConfirmDialog ref="exitDlgRef" v-slot="{ closeAs }">
    <p class="mt-2">You have unsaved data, it will be lost on exit. Are you sure?</p>
    <div class="mt-4">
      <ButtonNormal class="me-2" @click="closeAs('ok')">Save</ButtonNormal>
      <ButtonDanger class="me-2" @click="closeAs('dismiss')">Dismiss</ButtonDanger>
      <ButtonNormal class="me-2" @click="closeAs('cancel')">Cancel</ButtonNormal>
    </div>
  </ConfirmDialog>

</template>

<style scoped>
.bounce-enter-active {
  animation: bounce-in 0.2s;
}
.bounce-leave-active {
  animation: bounce-in 0.2s reverse;
}
@keyframes bounce-in {
  0% {
    transform: scale(0);
  }
  100% {
    transform: scale(1);
  }
}
</style>
