<script setup lang="ts">
import { inject, ref } from 'vue'
import { Device } from '@/device'
import { VirtualBackend } from '@/device/virtual_backend'
import { BleBackend } from '@/device/ble_backend'
import ButtonNormal from './buttons/ButtonNormal.vue'

const device: Device = inject('device')!
const has_bluetooth = ref(!!navigator.bluetooth)
</script>

<template>
    <div v-if="device.is_ready.value && (device.backend_id.value === VirtualBackend.id)" class="fixed bottom-1 right-2">
        <button
            type="button"
            class="text-white border border-red-700 bg-red-700 font-medium rounded-lg text-xs px-3 py-1.5 text-center me-2 mb-2"
            @click="device.selectBackend(BleBackend.id)"
        >
            Exit Demo Mode
        </button>
    </div>

    <div v-if="!device.is_ready.value" class="absolute w-full h-full px-4 py-4 bg-white text-slate-700">
        <button
            type="button"
            class="text-blue-900 bg-blue-200 underline font-medium text-xs w-full px-3 py-1.5 text-center mb-8"
            @click="device.selectBackend(VirtualBackend.id)"
        >
            Switch to Demo Mode
        </button>

        <!-- Show when Web Bluetooth not exists -->
        <div v-if="!has_bluetooth">
            <p class="mb-8 text-red-800">
                Web Bluetooth is disabled or not supported! See instruction below how to fix.
            </p>

            <div class="bg-green-50 border-t border-b border-green-400 text-green-700 px-4 py-3 mb-8">
                <p class="mb-2 font-bold">Chrome / Edge browsers:</p>
                <ul class="list-disc mx-6 text-sm mb-1">
                    <li>Type <b>chrome://flags</b> or <b>edge://flags</b> and press Enter.</li>
                    <li>Type <b>bluetooth</b> in search field.</li>
                    <li>Enable all found flags.</li>
                    <li>Restart your browser.</li>
                </ul>
            </div>

            <div class="bg-amber-50 border-t border-b border-amber-400 text-amber-700 px-4 py-3" role="alert">
                <p class="text-sm">Unforunately, Firefox and Safary do not support Web Bluetooth. Use browsers above, of try demo mode.</p>
            </div>
        </div>

        <!-- Show when Web Bluetooth exists -->
        <template v-else>
            <!-- Not connected => show button or progress message -->
            <template v-if="!device.is_connected.value">
                <p v-if="device.is_connecting.value">
                    <span class="align-text-top mr-1 w-5 h-5 animate-spin inline-block border-[3px] border-current border-t-transparent rounded-full text-gray-400"></span>
                    Connecting...
                </p>
                <ButtonNormal v-else class="w-full" @click="device.connect()">Connect to device</ButtonNormal>
            </template>

            <!-- Connected => Needs authentication and then data sync -->
            <template v-else>
                <div v-if="!device.is_authenticated.value">
                    <p class="mb-4">
                        <span class="align-text-top mr-1 w-5 h-5 animate-spin inline-block border-[3px] border-current border-t-transparent rounded-full text-gray-400"></span>
                        Authenticating...
                    </p>
                    <p v-if="device.need_pairing.value" class="text-red-800">
                        Device is connected, but not paired. <b>Click device button 5 times quickly, to activate pairing mode</b>.
                    </p>
                </div>
                <p v-else>
                    <span class="align-text-top mr-1 w-5 h-5 animate-spin inline-block border-[3px] border-current border-t-transparent rounded-full text-gray-400"></span>
                    Read config from device...
                </p>
            </template>
        </template>
    </div>
</template>