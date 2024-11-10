import { BleRpcClient } from '../../../src/lib/ble/BleRpcClient';

if (!('bluetooth' in navigator)) {
    alert('Web Bluetooth API is not available in this browser.');
}

window.addEventListener('unhandledrejection', function(event) {
    console.error('Unhandled rejection:', event.reason);
});

const rpcClient = new BleRpcClient();

rpcClient.log = (...data) => { console.log(...data); };
rpcClient.log_error = (...data) => { console.error(...data); };

document.getElementById('connectButton')?.addEventListener('click', async () => {
    try {
        await rpcClient.selectDevice();
    } catch (error) {
        console.error(error);
    }
});

document.getElementById('disconnectButton')?.addEventListener('click', async () => {
    //connector.disconnect();
    alert('Not implemented');
});

document.getElementById('simpleCommandsButton')?.addEventListener('click', async () => {
    try {
        console.log(`Echo response: ${await rpcClient.invoke('echo', 'Hello, BLE!')}`);
    } catch (error) {
        console.error(error);
    }
});

document.getElementById('bigUploadButton')?.addEventListener('click', async () => {
    const total_size = 1024 * 1024;
    const block_size = 16 * 1024;

    const startTime = Date.now();

    for (let i = 0; i < total_size; i += block_size) {
        const block = 'A'.repeat(block_size);
        await rpcClient.invoke('devnull', block);
        console.log(`${new Date().toLocaleTimeString()} Sent ${i + block_size} bytes`);
    }

    const endTime = Date.now();

    console.log(`Done (${(endTime - startTime)/1000} seconds)`);
});