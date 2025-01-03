import { BleClientChunker, isLastChunk, type IO } from './BleClientChunker'
import { RpcCaller, type RpcArgument, type RpcResult } from './RpcCaller'
import { AuthStorage } from './AuthStorage'
import { decode as msgpack_decode } from '@msgpack/msgpack'

interface AuthInfo {
    id: Uint8Array;
    hmac_msg: Uint8Array;
    pairable: boolean;
}

type BleEvent = 'disconnected' | 'connected' | 'ready' | 'need_pairing' | 'status_changed';

export class BleRpcClient {
    private device: BluetoothDevice | null = null;
    private gattServer: BluetoothRemoteGATTServer | null = null;

    private rpcIO = new BleCharacteristicIO();
    private authIO = new BleCharacteristicIO();

    private rpcCaller = new RpcCaller(new BleClientChunker(this.rpcIO));
    private authCaller = new RpcCaller(new BleClientChunker(this.authIO));

    private authStorage = new AuthStorage();

    private emitter = new EventEmitter<BleEvent>();

    private isConnectedFlag = false;
    private isAuthenticatedFlag = false;
    private needPairingFlag = false;
    private boundedDisconnectHandler = this.handleDisconnection.bind(this);

    // UUIDs
    private static readonly SERVICE_UUID = '5f524546-4c4f-575f-5250-435f5356435f'; // _REFLOW_RPC_SVC_
    private static readonly RPC_CHARACTERISTIC_UUID = '5f524546-4c4f-575f-5250-435f494f5f5f'; // _REFLOW_RPC_IO__
    private static readonly AUTH_CHARACTERISTIC_UUID = '5f524546-4c4f-575f-5250-435f41555448'; // _REFLOW_RPC_AUTH

    log: (...data: unknown[]) => void = () => {};
    log_error: (...data: unknown[]) => void = () => {};

    constructor() {
        this.loop().catch(this.log_error);
    }

    isConnected() { return this.isConnectedFlag && this.gattServer?.connected === true; }
    isAuthenticated() { return this.isAuthenticatedFlag; }
    isReady() { return this.isConnected() && this.isAuthenticated(); }
    needPairing() { return this.needPairingFlag; }
    isDeviceSelected() { return this.device !== null; }

    on(event: BleEvent, listener: (data?: unknown) => void) { this.emitter.on(event, listener); }
    off(event: BleEvent, listener: (data?: unknown) => void) { this.emitter.off(event, listener); }
    once(event: BleEvent, listener: (data?: unknown) => void) { this.emitter.once(event, listener); }

    async invoke(method: string, ...args: RpcArgument[]): Promise<RpcResult> {
        if (!this.isReady()) throw new Error('DisconnectedError: Device is not ready');
        return await this.rpcCaller.invoke(method, ...args);
    }

    async selectDevice(once = false) {
        if (once && this.device) {
            this.log('Device already found.', this.device);
            return;
        }

        const device = await navigator.bluetooth.requestDevice({
            filters: [{ services: [BleRpcClient.SERVICE_UUID] }],
        });

        if (this.device) {
            // Cleanup old device
            this.device?.removeEventListener('gattserverdisconnected', this.boundedDisconnectHandler);
            this.cleanup();
        }

        this.device = device;
        this.device.addEventListener('gattserverdisconnected', this.boundedDisconnectHandler);
    }

    private lastConnectedTime = 0;
    private lastAuthenticatedTime = 0;
    private readonly CONNECT_DEBOUNCE_PERIOD = 5000; // in milliseconds
    private readonly AUTH_DEBOUNCE_PERIOD = 1000; // in milliseconds

    private async loop() {
        while (true) {
            if (!this.isConnectedFlag && this.device &&
                (Date.now() - this.lastConnectedTime > this.CONNECT_DEBOUNCE_PERIOD)) {
                try {
                    this.lastConnectedTime = Date.now();

                    const [rpcChar, authChar] = await this.connect();

                    if (this.device?.gatt?.connected) {
                        this.rpcIO.setCharacteristic(rpcChar);
                        this.authIO.setCharacteristic(authChar);

                        // Init flags to "just connected" state
                        this.isConnectedFlag = true;
                        this.needPairingFlag = false;
                        this.isAuthenticatedFlag = false;

                        // Reset debounce delays after successful connection
                        this.lastConnectedTime = 0;
                        this.lastAuthenticatedTime = 0;

                        this.emitter.emit('connected');
                        this.emitter.emit('status_changed');
                    }
                } catch (error) {
                    this.log_error('Error:', error);
                }
            }

            if (this.isConnectedFlag && !this.isAuthenticatedFlag &&
                (Date.now() - this.lastAuthenticatedTime > this.AUTH_DEBOUNCE_PERIOD)) {
                try {
                    this.lastAuthenticatedTime = Date.now();
                    const successful = await this.authenticate();

                    if (successful) {
                        this.isAuthenticatedFlag = true;
                        this.needPairingFlag = false;
                        this.emitter.emit('ready');
                        this.emitter.emit('status_changed');
                    } else {
                        this.needPairingFlag = true;
                        this.emitter.emit('need_pairing');
                        this.emitter.emit('status_changed');
                    }
                } catch (error) {
                    this.log_error('Error:', error);
                }
            }

            await this.delay(200);
        }
    }

    private async connect(): Promise<Array<BluetoothRemoteGATTCharacteristic>> {

        this.log(`Connecting to GATT Server on ${this.device?.name}...`);

        this.gattServer = (await this.device?.gatt?.connect()) ?? null;
        if (!this.gattServer) {
            throw new Error('Failed to connect to GATT server.');
        }

        // Get primary service
        const services = await this.gattServer.getPrimaryServices();
        if (services?.length !== 1) {
            throw new Error(`Bad amount of services (${services.length}).`);
        }
        const service = services[0];

        // Get characteristics
        const rpcCharacteristic = await service.getCharacteristic(BleRpcClient.RPC_CHARACTERISTIC_UUID)
        const authCharacteristic = await service.getCharacteristic(BleRpcClient.AUTH_CHARACTERISTIC_UUID)
        if (!rpcCharacteristic || !authCharacteristic) {
            throw new Error('Failed to get characteristics.');
        }

        this.log('BLE connected (but needs authentication to become ready)');
        return [rpcCharacteristic, authCharacteristic];
    }

    private async authenticate(): Promise<boolean> {
        try {
            const client_id: Uint8Array = this.authStorage.getClientId();

            let auth_info : AuthInfo = msgpack_decode(await this.authCaller.invoke('auth_info') as Uint8Array) as AuthInfo
            const device_id = auth_info.id;
            let secret: Uint8Array;

            if (!this.authStorage.hasSecret(device_id)) {
                if (!auth_info.pairable) return false;

                this.log('Try to pair...');

                const new_secret  = await this.authCaller.invoke('pair', client_id) as Uint8Array;
                this.authStorage.setSecret(device_id, new_secret);
                secret = new_secret;
                // Re-fetch new hmac message value
                auth_info = msgpack_decode(await this.authCaller.invoke('auth_info') as Uint8Array) as AuthInfo;

                this.log('Paired!');
            } else {
                secret = this.authStorage.getSecret(device_id);
            }

            this.log('Authenticating...');

            const signature = await this.authStorage.calculateHMAC(auth_info.hmac_msg, secret);
            const authenticated = await this.authCaller.invoke('authenticate', client_id, signature, Date.now()) as boolean;

            if (!authenticated) {
                // Wrong key. Drop it.
                this.log('Wrong auth key. Clearing...');

                this.authStorage.setSecret(device_id, null);
                return false;
            }

            this.log('RPC ready!');
            return true;
        } catch (error) {
            this.log_error('Error:', error);
        }

        return false;
    }

    private async cleanup() {
        this.isConnectedFlag = false;
        this.isAuthenticatedFlag = false;

        // Clear characteristics in IO objects
        this.rpcIO.setCharacteristic(null);
        this.authIO.setCharacteristic(null);
    }

    private async handleDisconnection(): Promise<void> {
        this.log('Disconnected from GATT server.');
        this.cleanup();
        this.emitter.emit('disconnected');
        this.emitter.emit('status_changed');
    }

    private delay(ms: number): Promise<void> {
        return new Promise((resolve) => setTimeout(resolve, ms));
    }
}

/**
 * Helper class to adapt a BluetoothRemoteGATTCharacteristic to the IO interface.
 */
class BleCharacteristicIO implements IO {
    private characteristic: BluetoothRemoteGATTCharacteristic | null = null;

    constructor(characteristic?: BluetoothRemoteGATTCharacteristic | null) {
        this.characteristic = characteristic || null;
    }

    setCharacteristic(characteristic: BluetoothRemoteGATTCharacteristic | null): void {
        this.characteristic = characteristic;
    }

    async write(data: Uint8Array): Promise<void> {
        if (!this.characteristic) {
            throw new Error('DisconnectedError: No characteristic available for writing');
        }
        if (isLastChunk(data)) {
            // Last chunk delivery is guaranteed.
            await this.characteristic.writeValueWithResponse(data);
        } else {
            // Intermediate chunk delivery is best-effort.
            await this.characteristic.writeValueWithoutResponse(data);
        }
    }

    async read(): Promise<Uint8Array> {
        if (!this.characteristic) {
            throw new Error('DisconnectedError: No characteristic available for reading');
        }
        const value = await this.characteristic.readValue();
        return new Uint8Array(value.buffer);
    }
}

class EventEmitter<T extends string> {
    private events: { [key in T]?: Array<(data?: unknown) => void> } = {};

    on(event: T, listener: (data?: unknown) => void): void {
        if (!this.events[event]) this.events[event] = [];
        this.events[event]?.push(listener);
    }

    off(event: T, listenerToRemove: (data?: unknown) => void): void {
        if (!this.events[event]) return;
        this.events[event] = this.events[event]?.filter(listener => listener !== listenerToRemove);
    }

    emit(event: T, data?: unknown): void {
        if (!this.events[event]) return;
        this.events[event]?.forEach(listener => listener(data));
    }

    once(event: T, listener: (data?: unknown) => void): void {
        const onceListener = (data?: unknown) => {
            this.off(event, onceListener);
            listener(data);
        };
        this.on(event, onceListener);
    }
}