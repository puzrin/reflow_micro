<script setup lang="ts">
import filenamify from 'filenamify/browser'
import ky from 'ky'
import { type Profile } from '@/proto/generated/types'
import { useProfilesStore } from '@/stores/profiles'
import { THEME_MODES, normalizeThemeMode, type ThemeMode, useLocalSettingsStore } from '@/stores/localSettings'
import { computed, ref, inject, onBeforeUnmount, onMounted } from 'vue'
import { VueDraggable, type SortableEvent } from 'vue-draggable-plus'
import { useRouter } from 'vue-router'
import { Device } from '@/device'
import { confirm } from '@/composables/confirm'
import { notify } from '@/composables/notify'
import { usePageShell } from '@/composables/appShell'
import { decodePayload, encodeProfile } from '@/lib/exchange_format'
import { normalizeProfileUrl } from '@/lib/normalize_profile_url'
import { SharedConstants as Constants } from '@/lib/shared_constants'

const device: Device = inject('device')!
const router = useRouter()
const textEncoder = new TextEncoder()

const profilesStore = useProfilesStore()
const localSettingsStore = useLocalSettingsStore()

const themeModeTitles: Record<ThemeMode, string> = {
  auto: 'Auto',
  light: 'Light',
  dark: 'Dark',
}

const themeModeSubtitle = computed(() => themeModeTitles[localSettingsStore.themeMode])
const canAddProfiles = computed(() => profilesStore.items.length < Constants.MAX_REFLOW_PROFILES)
const reorderProfilesMode = ref(false)
const themeDialogOpen = ref(false)

usePageShell(() => ({
  title: 'Settings',
  nav: { kind: 'back', to: { name: 'home' } },
  pageMode: 'default',
}))

function toggleReorderProfilesMode() {
  closeImportProfilesMode()
  reorderProfilesMode.value = !reorderProfilesMode.value
}

function openThemeDialog() {
  themeDialogOpen.value = true
}

function closeThemeDialog() {
  themeDialogOpen.value = false
}

function selectThemeMode(mode: ThemeMode) {
  localSettingsStore.themeMode = normalizeThemeMode(mode)
  closeThemeDialog()
}

async function copyProfileToClipboard(profileId: number) {
  try {
    const profile = profilesStore.find(profileId)
    if (!profile) throw new Error('Profile not found')

    const profileJson = encodeProfile(profile)

    if (!navigator.clipboard) throw new Error('Clipboard API is not available')
    await navigator.clipboard.writeText(profileJson)

    notify({ message: 'Profile copied to clipboard', color: 'success' })
  } catch {
    notify({ message: 'Failed to copy', color: 'error' })
  }
}

function downloadProfile(profileId: number) {
  try {
    const profile = profilesStore.find(profileId)
    if (!profile) throw new Error('Profile not found')

    const profileJson = encodeProfile(profile)
    const downloadName = filenamify(`${profile.name}.json`)

    const blob = new Blob([profileJson], { type: 'application/json;charset=utf-8' })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')

    link.href = url
    link.download = downloadName
    link.style.display = 'none'

    document.body.appendChild(link)
    link.click()
    link.remove()

    window.setTimeout(() => URL.revokeObjectURL(url), 0)
  } catch {
    notify({ message: 'Failed to download', color: 'error' })
  }
}

async function deleteProfile(profileId: number) {
  const result = await confirm({
    title: 'Remove profile?',
    cancelKey: 'dismiss',
    actions: [
      { key: 'dismiss', label: 'Dismiss' },
      { key: 'confirm', label: 'Confirm', color: 'primary' },
    ],
  })

  if (result === 'confirm') profilesStore.remove(profileId)
}

async function resetProfiles() {
  const result = await confirm({
    title: 'Reset profiles?',
    description: 'All custom changes to your heating profiles will be lost.',
    cancelKey: 'dismiss',
    actions: [
      { key: 'dismiss', label: 'Dismiss' },
      { key: 'confirm', label: 'Confirm', color: 'primary' },
    ],
  })

  if (result === 'confirm') {
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

async function saveBleName() {
  bleNameError.value = false

  if (!bleNameDraft.value.length) {
    bleNameError.value = true
    return
  }

  if (textEncoder.encode(bleNameDraft.value).length > Constants.MAX_BLE_NAME_LENGTH) {
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
    notify({ message: 'Failed to update Bluetooth name', color: 'error' })
  }
}

// Import profiles

const importProfilesMode = ref(false)
const isImportDragOver = ref(false)
const urlImportDialogOpen = ref(false)
const urlImportDraft = ref('')
const urlImportLoading = ref(false)
const importFileInput = ref<HTMLInputElement | null>(null)

onMounted(() => {
  document.addEventListener('paste', handleGlobalPaste)
})

onBeforeUnmount(() => {
  document.removeEventListener('paste', handleGlobalPaste)
})

function openImportProfilesMode() {
  if (!canAddProfiles.value) return
  reorderProfilesMode.value = false
  importProfilesMode.value = true
}

function closeImportProfilesMode() {
  importProfilesMode.value = false
  isImportDragOver.value = false
  urlImportDialogOpen.value = false
  urlImportLoading.value = false
}

function openUrlImportDialog() {
  urlImportDraft.value = ''
  urlImportDialogOpen.value = true
}

function closeUrlImportDialog() {
  urlImportDialogOpen.value = false
  urlImportDraft.value = ''
}

function showImportError(error: unknown) {
  notify({
    message: error instanceof Error && error.message ? error.message : 'Failed to import profile',
    color: 'error',
  })
}

function importProfile(profile: Profile) {
  if (!canAddProfiles.value) {
    notify({ message: `Only ${Constants.MAX_REFLOW_PROFILES} profiles are allowed`, color: 'error' })
    return
  }

  profilesStore.add(profile)
  notify({ message: `Imported "${profile.name}"`, color: 'success' })
}

async function importFromUrl() {
  const url = urlImportDraft.value.trim()
  if (!url || urlImportLoading.value) return

  urlImportLoading.value = true

  try {
    const payload = await ky(await normalizeProfileUrl(url), { retry: 0 }).text()
    importProfile(decodePayload(payload).profile)
  } catch (error) {
    showImportError(error)
  } finally {
    urlImportLoading.value = false
    urlImportDialogOpen.value = false
    urlImportDraft.value = ''
  }
}

async function handleImportFileSelection(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0] ?? null

  input.value = ''
  if (!file) return

  try {
    const payload = await file.text()
    importProfile(decodePayload(payload).profile)
  } catch (error) {
    showImportError(error)
  }
}

function handleImportDragOver(event: DragEvent) {
  if (!importProfilesMode.value) return
  event.preventDefault()
  isImportDragOver.value = true
}

function handleImportDragLeave(event: DragEvent) {
  const currentTarget = event.currentTarget as HTMLElement | null
  const relatedTarget = event.relatedTarget as Node | null
  if (currentTarget?.contains(relatedTarget)) return
  isImportDragOver.value = false
}

async function getProfileFromEvent(event: DragEvent | ClipboardEvent): Promise<Profile> {
  const transfer = 'clipboardData' in event ? event.clipboardData : event.dataTransfer
  if (!transfer) throw new Error('No import data found')

  const items = Array.from(transfer.items)
  const fileEntries = items.filter((item) => item.kind === 'file')

  if (fileEntries.length > 1) throw new Error('Import only one file at a time')

  if (fileEntries.length === 1) {
    const file = fileEntries[0].getAsFile()

    if (!file) throw new Error('Failed to read file')

    return decodePayload(await file.text()).profile
  }

  // Without file entries, the transfer can only resolve as plain text or no usable import data.
  const payload = transfer.getData('text/plain')
  if (!payload) throw new Error('No import data found')

  return decodePayload(payload).profile
}

async function handleImportDrop(event: DragEvent) {
  if (!importProfilesMode.value || !event.dataTransfer) return

  event.preventDefault()
  isImportDragOver.value = false

  try {
    importProfile(await getProfileFromEvent(event))
  } catch (error) {
    showImportError(error)
  }
}

async function handleGlobalPaste(event: ClipboardEvent) {
  if (!importProfilesMode.value) return
  if (urlImportDialogOpen.value) return
  if (
    event.target instanceof HTMLElement
    && (event.target.isContentEditable
      || !!event.target.closest('input, textarea, [contenteditable=""], [contenteditable="true"]'))
  ) return
  if (!event.clipboardData) return

  event.preventDefault()

  try {
    const profile = await getProfileFromEvent(event)

    const result = await confirm({
      title: 'Import profile?',
      description: profile.name,
      cancelKey: 'dismiss',
      actions: [
        { key: 'dismiss', label: 'Dismiss' },
        { key: 'confirm', label: 'Confirm', color: 'primary' },
      ],
    })

    if (result === 'confirm') {
      importProfile(profile)
    }
  } catch (error) {
    showImportError(error)
  }
}

</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-card>
      <v-card-item class="pr-1" title="Heating profiles">
        <template #append>
          <v-menu location="bottom end">
            <template #activator="{ props }">
              <v-btn
                v-bind="props"
                icon="i-material-symbols:more-vert"
                variant="text"
                aria-label="Heating profile actions"
              />
            </template>

            <v-list>
              <v-list-item title="Import" :disabled="!canAddProfiles" @click="importProfilesMode ? closeImportProfilesMode() : openImportProfilesMode()" />
              <v-list-item title="Reorder" @click="toggleReorderProfilesMode" />
              <v-list-item title="Reset profiles" @click="resetProfiles" />
            </v-list>
          </v-menu>
        </template>
      </v-card-item>
      <v-divider />
      <v-card-text v-if="importProfilesMode" class="pt-4">
        <v-alert
          class="border"
          :color="isImportDragOver ? 'warning' : 'primary'"
          variant="tonal"
          closable
          text="Drop a file or text here, paste here with Ctrl/Cmd+V, or use one of the actions below."
          @click:close="closeImportProfilesMode"
          @dragenter.prevent="isImportDragOver = true"
          @dragover="handleImportDragOver"
          @dragleave="handleImportDragLeave"
          @drop="handleImportDrop"
        >
          <div class="d-flex flex-wrap justify-center ga-3 mt-4">
            <v-btn variant="outlined" @click="importFileInput?.click()">From file</v-btn>
            <v-btn variant="outlined" @click="openUrlImportDialog">From URL</v-btn>
          </div>
        </v-alert>
      </v-card-text>
      <v-divider v-if="importProfilesMode" />
      <VueDraggable
        v-model="profilesStore.items"
        tag="div"
        target=".profiles-list"
        :animation="150"
        :disabled="!reorderProfilesMode"
        handle=".profile-handle"
        :customUpdate="(evt: SortableEvent) => profilesStore.move(evt.oldIndex ?? 0, evt.newIndex ?? 0)"
      >
        <v-list class="profiles-list" lines="one">
          <v-list-item
            v-for="profile in profilesStore.items"
            :key="profile.id"
            link
            @click="router.push({ name: 'profile', params: { id: profile.id } })"
          >
            <v-list-item-title class="text-truncate">{{ profile.name }}</v-list-item-title>

            <template #append>
              <v-list-item-action end>
                <v-menu location="bottom end">
                  <template #activator="{ props }">
                    <v-btn
                      v-bind="props"
                      size="small"
                      icon="i-material-symbols:more-vert"
                      variant="text"
                      aria-label="Profile actions"
                      @click.stop
                      @mousedown.stop
                    />
                  </template>

                  <v-list>
                    <v-list-item
                      title="Duplicate"
                      :disabled="!canAddProfiles"
                      @click="router.push({ name: 'profile', params: { id: 0 }, query: { source_profile_id: String(profile.id) } })"
                    />
                    <v-list-item
                      title="Copy to clipboard"
                      @click="copyProfileToClipboard(profile.id)"
                    />
                    <v-list-item
                      title="Download"
                      @click="downloadProfile(profile.id)"
                    />
                    <v-list-item
                      title="Remove"
                      :disabled="profilesStore.items.length < 2"
                      @click="deleteProfile(profile.id)"
                    />
                  </v-list>
                </v-menu>
                <v-btn
                  v-if="reorderProfilesMode"
                  class="profile-handle"
                  icon="i-material-symbols:drag-indicator"
                  size="small"
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
        <v-btn color="primary" :to="{ name: 'profile', params: { id: 0 } }" :disabled="!canAddProfiles">Add</v-btn>
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
          link
          @click="openThemeDialog"
        />
        <v-list-item
          title="Show debug info"
          :subtitle="localSettingsStore.showDebugInfo ? 'On' : 'Off'"
          @click="localSettingsStore.showDebugInfo = !localSettingsStore.showDebugInfo"
        >
          <template #append>
            <v-switch
              color="primary"
              true-icon="i-material-symbols:check"
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
        <v-list-item
          title="Measure resistance"
          :to="{ name: 'measure_resistance' }"
        />
        <v-list-item
          title="List PD profiles"
          :to="{ name: 'pd_profiles' }"
        />
        </v-list>
      </v-card>
  </v-container>

  <v-dialog v-model="themeDialogOpen" max-width="30rem">
    <v-card>
      <v-card-item title="Theme" />
      <v-list lines="one">
        <v-list-item
          v-for="mode in THEME_MODES"
          :key="mode"
          :title="themeModeTitles[mode]"
          @click="selectThemeMode(mode)"
        >
          <template #prepend>
            <v-radio
              :model-value="localSettingsStore.themeMode"
              :value="mode"
              color="primary"
            />
          </template>
        </v-list-item>
      </v-list>
      <v-card-actions>
        <v-btn @click="closeThemeDialog">Dismiss</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <v-dialog v-model="bleNameDialogOpen" max-width="30rem">
    <v-card>
      <v-card-item title="Bluetooth name" />
      <v-card-text>
        <v-text-field
          v-model="bleNameDraft"
          label="Bluetooth name"
          :maxlength="Constants.MAX_BLE_NAME_LENGTH"
          :error-messages="bleNameError ? [`Name must be non-empty printable ASCII up to ${Constants.MAX_BLE_NAME_LENGTH} bytes`] : []"
        />
      </v-card-text>
      <v-card-actions>
        <v-btn @click="closeBleNameDialog">Dismiss</v-btn>
        <v-btn color="primary" @click="saveBleName">Save</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <v-dialog
    v-model="urlImportDialogOpen"
    max-width="30rem"
    :persistent="urlImportLoading"
  >
    <v-card>
      <v-card-item title="Import from URL" />
      <v-card-text>
        <v-text-field
          v-model="urlImportDraft"
          label="URL"
          type="url"
          placeholder="https://example.com/profile.json"
        />
      </v-card-text>
      <v-card-actions>
        <v-btn :disabled="urlImportLoading" @click="closeUrlImportDialog">Dismiss</v-btn>
        <v-btn
          color="primary"
          :loading="urlImportLoading"
          :disabled="!urlImportDraft.trim()"
          @click="importFromUrl"
        >
          Import
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <input
    ref="importFileInput"
    class="d-none"
    type="file"
    accept="application/json,.json"
    @change="handleImportFileSelection"
  >
</template>

<style scoped>
.profile-handle {
  cursor: grab;
}

.profile-handle:active {
  cursor: grabbing;
}
</style>
