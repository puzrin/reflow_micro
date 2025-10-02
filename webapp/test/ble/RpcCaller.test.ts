import { test, expect } from 'vitest';
import { RpcCaller } from '../../src/lib/ble/RpcCaller';
import { type BinaryTransport } from '../../src/lib/ble/BleClientChunker';
import { encode } from '@msgpack/msgpack';

function json2msgp(str: string): Uint8Array {
    return encode(JSON.parse(str));
}

class MockTransport implements BinaryTransport {
    private readResponses: Uint8Array[];
    private writes: Uint8Array[];

    constructor(readResponses: Uint8Array[]) {
        this.readResponses = readResponses;
        this.writes = [];
    }

    async send(data: Uint8Array): Promise<Uint8Array> {
        this.writes.push(data);
        if (this.readResponses.length === 0) {
            throw new Error('No more data to read');
        }
        return this.readResponses.shift()!;
    }

    getWrites(): Uint8Array[] {
        return this.writes;
    }
}

test('RpcCaller should correctly send a request and receive a successful response', async () => {
    const mockResponse = json2msgp('{ "ok": true, "result": 42 }');
    const transport = new MockTransport([mockResponse]);
    const rpcClient = new RpcCaller(transport);

    const result = await rpcClient.invoke('someMethod', 1, 2, 3);

    expect(result).toBe(42);
});

test('RpcCaller should throw an error when the response contains an error', async () => {
    const mockResponse = json2msgp('{ "ok": false, "result": "Error message" }');
    const transport = new MockTransport([mockResponse]);
    const rpcClient = new RpcCaller(transport);

    await expect(rpcClient.invoke('someMethod', 1, 2, 3)).rejects.toThrow(/RPC Error: Error message/);
});

test('RpcCaller should correctly handle different argument types', async () => {
    const mockResponse = json2msgp('{ "ok": true, "result": "success" }');
    const transport = new MockTransport([mockResponse]);
    const rpcClient = new RpcCaller(transport);

    const result = await rpcClient.invoke('anotherMethod', true, 123, 'test');

    expect(result).toBe('success');

    // Check the sent data
    const writes = transport.getWrites();
    expect(writes).toHaveLength(1);
    expect(writes[0]).toEqual(json2msgp('{"method":"anotherMethod","args":[true,123,"test"]}'));
});

test('RpcCaller should handle empty argument list', async () => {
    const mockResponse = json2msgp('{ "ok": true, "result": "empty" }');
    const transport = new MockTransport([mockResponse]);
    const rpcClient = new RpcCaller(transport);

    const result = await rpcClient.invoke('methodWithoutArgs');

    expect(result).toBe('empty');

    // Check the sent data
    const writes = transport.getWrites();
    expect(writes).toHaveLength(1);
    expect(writes[0]).toEqual(json2msgp('{"method":"methodWithoutArgs","args":[]}'));
});

test('RpcCaller should handle unicode characters correctly', async () => {
    const mockResponse = json2msgp('{ "ok": true, "result": "успех" }');
    const transport = new MockTransport([mockResponse]);
    const rpcClient = new RpcCaller(transport);

    const result = await rpcClient.invoke('unicodeMethod', 'тест');

    expect(result).toBe('успех');

    // Check the sent data
    const writes = transport.getWrites();
    expect(writes).toHaveLength(1);
    expect(writes[0]).toEqual(json2msgp('{"method":"unicodeMethod","args":["тест"]}'));
});
