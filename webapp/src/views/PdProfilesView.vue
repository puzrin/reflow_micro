<script setup lang="ts">
import { usePageShell } from '@/composables/appShell'
import { Device } from '@/device'
import { PowerStatus } from '@/proto/generated/types'
import { computed, inject, onBeforeUnmount, onMounted, ref, watch } from 'vue'

const device: Device = inject('device')!

const status = device.status
const pdProfiles = ref<number[]>([])
let refreshTimerId: number | null = null

function canShowProfiles() {
  return status.power === PowerStatus.PWR_OK || status.power === PowerStatus.PWR_TRANSITION
}

const powerAlertText = computed(() => {
  switch (status.power) {
    case PowerStatus.PWR_OFF:
      return 'Power supply is off'
    case PowerStatus.PWR_INITIALIZING:
      return 'Power supply is initializing'
    case PowerStatus.PWR_FAILURE:
      return 'No suitable power supply detected'
    default:
      return ''
  }
})

const showPowerAlert = computed(() => !canShowProfiles())

usePageShell(() => ({
  title: 'List PD profiles',
  nav: { kind: 'back', to: { name: 'settings' } },
  pageMode: 'default',
}))

function formatPdo(value: number): string {
  return `0x${value.toString(16).toUpperCase().padStart(8, '0')}`
}

type DecodedPdo = {
  state: 'placeholder' | 'unknown' | 'fixed' | 'pps' | 'spr_avs' | 'epr_avs'
  typeLabel: '' | 'FIXED' | 'PPS' | 'APDO_SPR_AVS' | 'APDO_EPR_AVS'
  mvMin: number
  mvMax: number
  ma: number
  pdp: number
}

function decodePdo(value: number): DecodedPdo {
  if (value === 0) {
    return { state: 'placeholder', typeLabel: '', mvMin: 0, mvMax: 0, ma: 0, pdp: 0 }
  }

  const pdoType = (value >>> 30) & 0b11
  if (pdoType === 0) {
    const mv = ((value >>> 10) & 0x3ff) * 50
    const ma = (value & 0x3ff) * 10
    return { state: 'fixed', typeLabel: 'FIXED', mvMin: mv, mvMax: mv, ma, pdp: 0 }
  }

  if (pdoType === 0b11) {
    const apdoSubtype = (value >>> 28) & 0b11

    if (apdoSubtype === 0) {
      const mvMin = ((value >>> 8) & 0xff) * 100
      const mvMax = ((value >>> 17) & 0xff) * 100
      const ma = (value & 0x7f) * 50
      return { state: 'pps', typeLabel: 'PPS', mvMin, mvMax, ma, pdp: 0 }
    }

    if (apdoSubtype === 2) {
      const ma15v = (value & 0x3ff) * 10
      const ma20v = ((value >>> 10) & 0x3ff) * 10
      const mvMax = ma20v > 0 ? 20000 : 15000
      const ma = ma20v > 0 ? ma20v : ma15v
      return { state: 'spr_avs', typeLabel: 'APDO_SPR_AVS', mvMin: 9000, mvMax, ma, pdp: 0 }
    }

    if (apdoSubtype === 1) {
      const pdp = (value & 0xff)
      const mvMin = ((value >>> 8) & 0xff) * 100
      const mvMax = ((value >>> 17) & 0x1ff) * 100
      return { state: 'epr_avs', typeLabel: 'APDO_EPR_AVS', mvMin, mvMax, ma: 0, pdp }
    }
  }

  return { state: 'unknown', typeLabel: '', mvMin: 0, mvMax: 0, ma: 0, pdp: 0 }
}

async function loadPdProfiles() {
  if (!device.is_ready.value || !canShowProfiles()) return

  try {
    pdProfiles.value = await device.get_pd_profiles()
  } catch {
    pdProfiles.value = []
  }
}

watch(
  () => device.is_ready.value,
  async (ready) => {
    if (!ready) {
      pdProfiles.value = []
      return
    }

    await loadPdProfiles()
  },
  { immediate: true }
)

watch(
  () => status.power,
  (power) => {
    if (power !== PowerStatus.PWR_OK && power !== PowerStatus.PWR_TRANSITION) {
      pdProfiles.value = []
    } else {
      void loadPdProfiles()
    }
  }
)

onMounted(() => {
  // Poll slowly enough to avoid visible churn, but keep PD changes fresh.
  refreshTimerId = window.setInterval(() => {
    void loadPdProfiles()
  }, 1000)
})

onBeforeUnmount(() => {
  if (refreshTimerId !== null) {
    window.clearInterval(refreshTimerId)
    refreshTimerId = null
  }
})
</script>

<template>
  <v-container class="py-4 d-flex flex-column ga-4">
    <v-alert
      v-if="showPowerAlert"
      type="error"
      :text="powerAlertText"
    />

    <v-card v-else>
      <v-list lines="two">
        <v-list-item
          v-if="pdProfiles.length === 0"
          title="--"
          subtitle="0x00000000"
        />
        <template v-for="(profile, index) in pdProfiles" :key="`${index}-${profile}`">
          <v-list-item>
            <v-list-item-title>
              <template v-if="decodePdo(profile).state === 'placeholder'">
                [{{ index + 1 }}] Placeholder
              </template>
              <template v-else-if="decodePdo(profile).state === 'unknown'">
                [{{ index + 1 }}] UNKNOWN
              </template>
              <template v-else>
                [{{ index + 1 }}] {{ decodePdo(profile).typeLabel }}
                <template v-if="decodePdo(profile).mvMin === decodePdo(profile).mvMax">
                  {{ decodePdo(profile).mvMin }} mV
                </template>
                <template v-else>
                  {{ decodePdo(profile).mvMin }}-{{ decodePdo(profile).mvMax }} mV
                </template>
                ,
                <template v-if="decodePdo(profile).state === 'epr_avs'">
                  {{ decodePdo(profile).pdp }} W
                </template>
                <template v-else>
                  {{ decodePdo(profile).ma }} mA
                </template>
              </template>
            </v-list-item-title>
            <v-list-item-subtitle>{{ formatPdo(profile) }}</v-list-item-subtitle>
          </v-list-item>
          <v-divider v-if="index < pdProfiles.length - 1" />
        </template>
      </v-list>
    </v-card>
  </v-container>
</template>
