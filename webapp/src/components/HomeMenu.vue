<script setup lang="ts">
import { useProfilesStore } from '@/stores/profiles'
import { inject } from 'vue'
import { DeviceActivityStatus } from '@/proto/generated/types'
import { Device } from '@/device'

const profilesStore = useProfilesStore()
const device: Device = inject('device')!
const status = device.status

const repoUrl = __REPO_URL__
const locked = () => status.activity === DeviceActivityStatus.REFLOW

</script>

<template>
  <v-menu location="bottom start" offset="10">
    <template #activator="{ props }">
      <v-app-bar-nav-icon v-bind="props" />
    </template>

    <v-list min-width="280">
      <v-list-item
        v-for="profile in profilesStore.items"
        :key="profile.id"
        :disabled="locked()"
        :title="profile.name"
        @click="profilesStore.select(profile.id)"
      >
        <template #prepend>
          <v-icon
            icon="i-material-symbols:check"
            :class="{ 'opacity-0': profile.id !== profilesStore.selected?.id }"
          />
        </template>
      </v-list-item>

      <v-divider class="my-1" />

      <v-list-item
        :disabled="locked()"
        title="Settings"
        :to="{ name: 'settings' }"
      >
        <template #prepend>
          <v-icon class="opacity-0" icon="i-material-symbols:check" />
        </template>
      </v-list-item>
      <v-list-item
        v-if="repoUrl"
        title="GitHub"
        :href="repoUrl"
        target="_blank"
      >
        <template #prepend>
          <v-icon class="opacity-0" icon="i-material-symbols:check" />
        </template>
      </v-list-item>
    </v-list>
  </v-menu>
</template>
