<script setup lang="ts">
import { useProfilesStore } from '@/stores/profiles'
import { THEME_MODES, normalizeThemeMode, type ThemeMode, useLocalSettingsStore } from '@/stores/localSettings'
import { computed, ref, inject, onMounted } from 'vue'
import { VueDraggable, type SortableEvent } from 'vue-draggable-plus'
import { useRouter } from 'vue-router'
import { Device } from '@/device'
import { confirm } from '@/composables/confirm'
import { notify } from '@/composables/notify'
import { usePageShell } from '@/composables/appShell'

const device: Device = inject('device')!
const router = useRouter()

const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()

const themeModeTitles: Record<ThemeMode, string> = {
  auto: 'Auto',
  light: 'Light',
  dark: 'Dark',
}

const themeModeItems = THEME_MODES.map((mode) => ({
  title: themeModeTitles[mode],
  value: mode,
}))

const themeModeSubtitle = computed(() => themeModeTitles[localSettingsStore.themeMode])

usePageShell(() => ({
  title: 'Settings',
  nav: { kind: 'back', to: { name: 'home' } },
  pageMode: 'default',
}))

// On drag, move the profile with the store method to keep reactivity working.
function customUpdate(evt: SortableEvent) {
  const { oldIndex, newIndex } = evt
  profilesStore.move(oldIndex ?? 0, newIndex ?? 0)
}

function openProfile(profileId: number) {
  router.push({ name: 'profile', params: { id: profileId } })
}

function duplicateProfile(profileId: number) {
  router.push({
    name: 'profile',
    params: { id: 0 },
    query: { source_profile_id: String(profileId) },
  })
}

async function deleteProfile(profileId: number) {
  const result = await confirm({
    title: 'Remove profile?',
    actions: [
      { key: 'remove', label: 'Remove', color: 'error' },
      { key: 'cancel', label: 'Cancel' },
    ],
  })

  if (result === 'remove') profilesStore.remove(profileId)
}

async function resetProfiles() {
  const result = await confirm({
    title: 'Reset profiles?',
    description: 'All custom changes to your heating profiles will be lost.',
    actions: [
      { key: 'reset', label: 'Reset', color: 'error' },
      { key: 'cancel', label: 'Cancel' },
    ],
  })

  if (result === 'reset') {
    device.loadProfilesData(true)
  }
}

// BLE name form

const bleName = ref('')
const bleNameDraft = ref('')
const bleNameError = ref(false)
const bleNameDialogOpen = ref(false)

onMounted(async () => {
  if (!device.is_ready.value) return
  bleName.value = await device.get_ble_name()
})

function openBleNameDialog() {
  bleNameDraft.value = bleName.value
  bleNameError.value = false
  bleNameDialogOpen.value = true
}

function closeBleNameDialog() {
  bleNameDialogOpen.value = false
  bleNameError.value = false
}

async function saveBleNameHandler() {
  bleNameError.value = false

  if (bleNameDraft.value.length < 3) {
    bleNameError.value = true
    return
  }

  // Check for ASCII only (printable characters 32-126)
  const isAscii = /^[\x20-\x7E]*$/.test(bleNameDraft.value)
  if (!isAscii) {
    bleNameError.value = true
    return
  }

  try {
    await device.set_ble_name(bleNameDraft.value)
    bleName.value = bleNameDraft.value
    bleNameDialogOpen.value = false
  } catch {
    bleNameError.value = true
    notify({ message: 'Failed to update', color: 'error' })
  }
}

</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-card>
      <v-card-item title="Heating profiles" />
      <v-divider />
      <VueDraggable
        v-model="profilesStore.items"
        tag="div"
        target=".profiles-list"
        :animation="150"
        handle=".profile-handle"
        :customUpdate="customUpdate"
      >
        <v-list class="profiles-list" lines="one">
          <v-list-item
            v-for="profile in profilesStore.items"
            :key="profile.id"
            link
            @click="openProfile(profile.id)"
          >
            <v-list-item-title class="text-truncate">{{ profile.name }}</v-list-item-title>

            <template #append>
              <v-list-item-action end>
                <v-menu location="bottom end">
                  <template #activator="{ props }">
                    <v-btn
                      v-bind="props"
                      size="x-small"
                      icon="mdi-dots-vertical"
                      variant="text"
                      aria-label="Profile actions"
                      @click.stop
                      @mousedown.stop
                    />
                  </template>

                  <v-list>
                    <v-list-item
                      title="Edit"
                      :to="{ name: 'profile', params: { id: profile.id } }"
                    />
                    <v-list-item
                      title="Duplicate"
                      @click="duplicateProfile(profile.id)"
                    />
                    <v-list-item
                      title="Remove"
                      :disabled="profilesStore.items.length < 2"
                      @click="deleteProfile(profile.id)"
                    />
                  </v-list>
                </v-menu>
                <v-btn
                  class="profile-handle"
                  icon="mdi-reorder-vertical"
                  size="x-small"
                  variant="text"
                  aria-label="Drag profile"
                  @click.stop
                  @mousedown.stop
                />
              </v-list-item-action>
            </template>
          </v-list-item>
        </v-list>
      </VueDraggable>
      <v-divider />
      <v-card-actions>
        <v-btn color="primary" variant="text" size="large" :to="{ name: 'profile', params: { id: 0 } }">Add</v-btn>
        <v-btn variant="text" size="large" @click="resetProfiles">Reset</v-btn>
      </v-card-actions>
    </v-card>

    <v-card>
      <v-card-item title="General" />
      <v-divider />
      <v-list lines="two">
        <v-list-item
          title="Bluetooth name"
          :subtitle="bleName || '--'"
          link
          @click="openBleNameDialog"
        />
        <v-list-item
          title="Theme"
          :subtitle="themeModeSubtitle"
        >
          <template #append>
            <div class="settings-select">
              <v-select
                :model-value="localSettingsStore.themeMode"
                :items="themeModeItems"
                item-title="title"
                item-value="value"
                @update:model-value="localSettingsStore.themeMode = normalizeThemeMode($event)"
              />
            </div>
          </template>
        </v-list-item>
        <v-list-item
          title="Show debug info"
          :subtitle="localSettingsStore.showDebugInfo ? 'On' : 'Off'"
          @click="localSettingsStore.showDebugInfo = !localSettingsStore.showDebugInfo"
        >
          <template #append>
            <v-switch
              :model-value="localSettingsStore.showDebugInfo"
              hide-details
              @click.stop
              @update:model-value="localSettingsStore.showDebugInfo = !!$event"
            />
          </template>
        </v-list-item>
      </v-list>
    </v-card>

    <v-card>
      <v-card-item title="Calibration" />
      <v-divider />
      <v-list lines="one">
        <v-list-item
          title="Calibrate temperature sensor"
          :to="{ name: 'calibrate_sensor' }"
        />
        <v-list-item
          title="Calibrate temperature controller"
          :to="{ name: 'calibrate_adrc' }"
        />
        </v-list>
      </v-card>
  </v-container>

  <v-dialog v-model="bleNameDialogOpen" max-width="30rem">
    <v-card>
      <v-card-item title="Bluetooth name" />
      <v-card-text>
        <v-text-field
          v-model="bleNameDraft"
          label="Bluetooth name"
          minlength="3"
          maxlength="30"
          :error-messages="bleNameError ? ['Name must be 3-30 ASCII characters'] : []"
        />
      </v-card-text>
      <v-card-actions>
        <v-btn @click="closeBleNameDialog">Cancel</v-btn>
        <v-btn color="primary" variant="elevated" @click="saveBleNameHandler">Save</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<style scoped>
.profile-handle {
  cursor: grab;
}

.profile-handle:active {
  cursor: grabbing;
}

.settings-select {
  width: 8rem;
}
</style>
