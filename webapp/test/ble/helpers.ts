export class MockIO {
    private readResponses: Uint8Array[];
    private writeCalls: Uint8Array[] = [];
    private readIndex: number = 0;

    constructor(readResponses: Uint8Array[]) {
        this.readResponses = readResponses;
    }

    async read(): Promise<Uint8Array> {
        if (this.readIndex < this.readResponses.length) {
            return this.readResponses[this.readIndex++];
        } else {
            throw new Error('No more data to read');
        }
    }

    async write(data: Uint8Array): Promise<void> {
        this.writeCalls.push(data);
    }

    getWrites(): Uint8Array[] {
        return this.writeCalls;
    }
}

export function bin(...args: (number | Uint8Array | number[])[]): Uint8Array {
    const result: number[] = [];

    for (const arg of args) {
        if (typeof arg === 'number') {
            result.push(arg);
        } else if (Array.isArray(arg)) {
            result.push(...arg);
        } else if (arg instanceof Uint8Array) {
            result.push(...Array.from(arg));
        }
    }

    return new Uint8Array(result);
}
