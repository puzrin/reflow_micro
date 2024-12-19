import { type BinaryTransport } from './BleClientChunker';
import { encode, decode } from '@msgpack/msgpack';

export type RpcArgument = boolean | number | string | Uint8Array;
export type RpcResult = boolean | number | string | Uint8Array;

export class RpcCaller {
    private transport: BinaryTransport;
    private queue: Promise<void> = Promise.resolve();

    constructor(transport: BinaryTransport) {
        this.transport = transport;
    }

    /**
     * Invokes an RPC method with the given arguments.
     */
    async invoke(method: string, ...args: RpcArgument[]): Promise<RpcResult> {
        const request = { method, args };
        const requestData = encode(request);

        // Chain the requests to ensure sequential processing
        const resultPromise = this.queue.then(async () => {
            const responseData = await this.transport.send(requestData);
            const response = decode(responseData) as { ok: boolean, result: RpcResult }

            if (response.ok !== true) {
                throw new Error(`RPC Error: ${response.result}`);
            }

            return response.result as RpcResult;
        });

        // Update the queue to reflect the current operation
        this.queue = resultPromise.then(() => {}).catch(() => {});

        return resultPromise;
    }
}
