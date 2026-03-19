import { type BinaryTransport } from './BleClientChunker';
import { encode, decode } from 'cbor-x';

export type RpcArgument = boolean | number | string | Uint8Array;
export type RpcResult = boolean | number | string | Uint8Array;

export class RpcCaller {
    private transport: BinaryTransport;
    private maxMessageSize: number;
    private queue: Promise<void> = Promise.resolve();

    constructor(transport: BinaryTransport, maxMessageSize: number) {
        this.transport = transport;
        this.maxMessageSize = maxMessageSize;
    }

    /**
     * Invokes an RPC method with the given arguments.
     */
    async invoke(method: string, ...args: RpcArgument[]): Promise<RpcResult> {
        const request = { method, params: args };
        const requestData = encode(request) as Uint8Array;

        if (requestData.length > this.maxMessageSize) {
            throw new Error(`RPC request is too large: ${requestData.length} > ${this.maxMessageSize}`);
        }

        // Chain the requests to ensure sequential processing
        const resultPromise = this.queue.then(async () => {
            const responseData = await this.transport.send(requestData);
            const response = decode(responseData) as { ok: boolean, result?: RpcResult, error?: string }

            if (response.ok !== true) {
                console.error('RPC invoke error =>', method, ...args, '=>', response.error);
                throw new Error(`RPC Error: ${response.error ?? 'Unknown error'}`);
            }

            return response.result as RpcResult;
        });

        // Update the queue to reflect the current operation
        this.queue = resultPromise.then(() => {}).catch(() => {});

        return resultPromise;
    }
}
