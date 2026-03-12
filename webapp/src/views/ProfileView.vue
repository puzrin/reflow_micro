<script setup lang="ts">
import { onBeforeRouteLeave } from 'vue-router'
import { useProfilesStore } from '@/stores/profiles'
import { useLocalSettingsStore } from '@/stores/localSettings'
import { computed, reactive, ref, toRaw, watch } from 'vue'
import { Profile, Constants } from '@/proto/generated/types'
import { confirm } from '@/composables/confirm'
import { notify } from '@/composables/notify'
import { usePageShell } from '@/composables/appShell'
import ReflowChart from '@/components/ReflowChart.vue'

// GUI field limits
const limits = {
  targetMin: 30,
  targetMax: 300,
  durationMin: 5,
  durationMax: 60*60*24,
  nameMin: 3,
  nameMax: 30
}

const props = defineProps<{ id: number }>()
const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()

// Load the profile from the store or create a new one
const srcProfile: Profile = profilesStore.exists(props.id) ?
  toRaw(profilesStore.find(props.id))! : {
    id: 0,
    name: '',
    segments: [
      { target: 150, duration: 60 }
    ]
  }

// Create a reactive clone to accumulate and track changes
const profile = reactive(structuredClone(srcProfile)!)
const isProfileEdited = ref(false)

usePageShell(() => ({
  title: profile.id ? 'Edit profile' : 'New profile',
  nav: { kind: 'back', to: { name: 'settings' } },
  pageMode: 'default',
}))

watch(profile, () => isProfileEdited.value = true, { deep: true })

const formRef = ref()
const previewPanels = computed({
  get: () => localSettingsStore.profileEditorShowPreview ? ['preview'] : [],
  set: (value: string[]) => {
    localSettingsStore.profileEditorShowPreview = value.includes('preview')
  },
})

const nameRules = [
  (value: string) => !!value || 'Name is required',
  (value: string) => value.length >= limits.nameMin || `Minimum ${limits.nameMin} characters`,
  (value: string) => value.length <= limits.nameMax || `Maximum ${limits.nameMax} characters`,
]

const targetRules = [
  (value: unknown) => Number(value) >= limits.targetMin || `Minimum ${limits.targetMin}°C`,
  (value: unknown) => Number(value) <= limits.targetMax || `Maximum ${limits.targetMax}°C`,
]

const durationRules = [
  (value: unknown) => Number(value) >= limits.durationMin || `Minimum ${limits.durationMin} sec`,
  (value: unknown) => Number(value) <= limits.durationMax || `Maximum ${limits.durationMax} sec`,
]

async function saveForm() {
  const validation = await formRef.value?.validate()
  if (!validation?.valid) return

  // Save the profile to the store and auto-update the ID to avoid duplicates
  // on the next save.
  profile.id = profilesStore.add(toRaw(profile))
  isProfileEdited.value = false
  notify({ message: 'Profile saved', color: 'success' })
}

onBeforeRouteLeave(async () => {
  if (!isProfileEdited.value) return true

  const result = await confirm({
    title: 'Save changes before leaving?',
    actions: [
      { key: 'save', label: 'Save', color: 'primary' },
      { key: 'discard', label: 'Discard', color: 'error' },
      { key: 'cancel', label: 'Cancel' },
    ],
  })

  if (result === 'cancel') return false
  if (result === 'discard') return true

  const validation = await formRef.value?.validate()
  if (validation?.valid) {
    await saveForm()
    return true
  }

  return false
})

function duplicateSegment(segmentIdx: number) {
  const newSegment = structuredClone(toRaw(profile).segments[segmentIdx])
  profile.segments.splice(segmentIdx, 0, newSegment)
}

async function deleteSegment(segmentIdx: number) {
  if (profile.segments.length < 2) return

  const result = await confirm({
    title: 'Remove stage?',
    actions: [
      { key: 'remove', label: 'Remove', color: 'error' },
      { key: 'cancel', label: 'Cancel' },
    ],
  })

  if (result === 'remove') {
    profile.segments.splice(segmentIdx, 1)
  }
}

function heatingSpeed(segmentIdx: number) {
  const start = segmentIdx === 0 ? Constants.START_TEMPERATURE : (profile.segments[segmentIdx - 1].target || 0)
  const segment = profile.segments[segmentIdx]
  const speed = (segment.target - start) / (segment.duration || 0)

  if (Math.abs(speed) < 0.0001) return `Keep`
  if (Math.abs(speed) > 0.1) return `${speed.toFixed(1)}°C/sec`
  return `${(speed * 60).toFixed(1)}°C/min`
}

</script>

<template>
  <v-container class="py-4">
    <v-form ref="formRef" class="d-flex flex-column ga-4" @submit.prevent="saveForm">
      <v-card>
        <v-card-text>
          <v-text-field
            v-model.trim="profile.name"
            label="Profile name"
            :rules="nameRules"
          />
        </v-card-text>
        <v-expansion-panels
          v-model="previewPanels"
          multiple
          variant="accordion"
        >
          <v-expansion-panel value="preview" static>
            <v-expansion-panel-title>Preview</v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-sheet class="chart-host chart-host--fixed-h flex-fill pa-4 rounded border">
                <div class="chart-host-wrap1">
                  <div class="chart-host-wrap2">
                    <ReflowChart id="profile-edit-chart" :profile="profile" />
                  </div>
                </div>
              </v-sheet>
            </v-expansion-panel-text>
          </v-expansion-panel>
        </v-expansion-panels>
      </v-card>

      <v-card>
        <v-card-item title="Stages" />
        <v-divider />
        <v-card-text class="d-flex flex-column ga-4">
          <div v-for="(segment, index) in profile.segments" :key="index">
            <div class="text-medium-emphasis mb-3">
              {{ heatingSpeed(index) }}, {{ segment.duration < 600 ? `${segment.duration} sec` : `${(segment.duration / 60).toFixed()} min` }}
            </div>
            <div class="d-flex flex-column flex-sm-row align-start ga-0 ga-sm-4">
              <div class="flex-1-1-0 w-100 w-sm-auto">
                <v-number-input
                  v-model="segment.target"
                  label="Target (°C)"
                  inset
                  :min="limits.targetMin"
                  :max="limits.targetMax"
                  :step="1"
                  :rules="targetRules"
                />
              </div>
              <div class="flex-1-1-0 w-100 w-sm-auto">
                <v-number-input
                  v-model="segment.duration"
                  label="Duration (sec)"
                  inset
                  :min="limits.durationMin"
                  :max="limits.durationMax"
                  :step="1"
                  :rules="durationRules"
                />
              </div>
              <div class="w-100 w-sm-auto d-flex justify-end justify-sm-start text-medium-emphasis pt-sm-3">
                <v-btn
                  variant="text"
                  icon="mdi-plus"
                  size="x-small"
                  @click="duplicateSegment(index)"
                />
                <v-btn
                  variant="text"
                  icon="mdi-delete-outline"
                  size="x-small"
                  :disabled="profile.segments.length < 2"
                  @click="deleteSegment(index)"
                />
              </div>
            </div>
          </div>
        </v-card-text>
        <v-card-actions>
          <v-btn color="primary" variant="text" size="large" type="submit">Save</v-btn>
        </v-card-actions>
      </v-card>
    </v-form>
  </v-container>
</template>
