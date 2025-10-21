<script setup lang="ts">
import { Menu, MenuButton, MenuItems, MenuItem } from '@headlessui/vue'
import { RouterLink } from 'vue-router'
import { useProfilesStore } from '@/stores/profiles'
import { inject } from 'vue'
import { DeviceActivityStatus } from '@/proto/generated/types'
import { Device } from '@/device'
import MenuIcon from '@heroicons/vue/24/outline/Bars4Icon'
import CheckIcon from '@heroicons/vue/24/outline/CheckIcon'

const profilesStore = useProfilesStore()
const device: Device = inject('device')!
const status = device.status

const repoUrl = __REPO_URL__

</script>

<template>
  <Menu as="div" class="relative">
    <MenuButton class="block mr-2 -ml-0.5">
      <MenuIcon class="w-8 h-8" />
    </MenuButton>

    <transition
      enter-active-class="transition duration-100 ease-out"
      enter-from-class="transform scale-95 opacity-0"
      enter-to-class="transform scale-100 opacity-100"
      leave-active-class="transition duration-75 ease-in"
      leave-from-class="transform scale-100 opacity-100"
      leave-to-class="transform scale-95 opacity-0"
    >
      <MenuItems
        class="absolute z-10 left-0 mt-2 text-base origin-top-left divide-y divide-gray-200 rounded-md bg-white shadow-lg ring-1 ring-black/5 focus:outline-none"
      >
        <div class="px-1 py-1">
          <template v-for="profile in profilesStore.items" :key="profile.id">
            <MenuItem v-slot="{ active }">
              <button
                class="flex w-full items-center rounded-md px-3 py-2 whitespace-nowrap text-ellipsis overflow-hidden disabled:opacity-50 disabled:pointer-events-none"
                :class="[
                  active ? 'bg-violet-500 text-white' : 'text-gray-900',
                  { 'disabled':  status.activity === DeviceActivityStatus.Reflow }
                ]"
                @click="() => profilesStore.select(profile.id)"
              >
                <span class="grow grid justify-start">{{ profile.name }}</span>
                <CheckIcon
                  :class="profile.id === profilesStore.selected?.id ? 'visible' : 'invisible'"
                  class="w-4 h-4 ms-2"
                />
              </button>
            </MenuItem>
          </template>
        </div>
        <div class="px-1 py-1">
          <MenuItem v-slot="{ active }">
            <RouterLink
              :to="{ name: 'settings' }"
              class="flex w-full items-center rounded-md px-3 py-2 whitespace-nowrap text-ellipsis overflow-hidden"
              :class="[
                active ? 'bg-violet-500 text-white' : 'text-gray-900',
                { 'disabled':  status.activity === DeviceActivityStatus.Reflow }
              ]"
            >
              Settings...
            </RouterLink>
          </MenuItem>
          <MenuItem v-if="repoUrl" v-slot="{ active }">
            <a
              class="flex w-full items-center rounded-md px-3 py-2 whitespace-nowrap text-ellipsis overflow-hidden"
              :class="active ? 'bg-violet-500 text-white' : 'text-gray-900'"
              :href="repoUrl"
              target="_blank"
            >
              Github
          </a>
          </MenuItem>
        </div>
      </MenuItems>
    </transition>
  </Menu>
</template>
