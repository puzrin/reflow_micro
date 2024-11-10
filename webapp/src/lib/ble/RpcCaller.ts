import { type BinaryTransport } from './BleClientChunker';

type RpcArgument = boolean | number | string;
type RpcResult = boolean | number | string;

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
        const requestData = new TextEncoder().encode(JSON.stringify(request));

        // Chain the requests to ensure sequential processing
        const resultPromise = this.queue.then(async () => {
            const responseData = await this.transport.send(requestData);
            const responseText = new TextDecoder().decode(responseData);
            const response = JSON.parse(responseText);

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
