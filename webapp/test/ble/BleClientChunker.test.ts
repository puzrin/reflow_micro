import { test, expect } from 'vitest';
import { BleClientChunker, BleChunkHead } from '../../src/lib/ble/BleClientChunker';
import { MockIO, bin } from './helpers';

test('BleClientChunker should handle single chunk response', async () => {
    const mockIO = new MockIO([bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [5, 6, 7, 8])]);

    const chunker = new BleClientChunker(mockIO);
    const result = await chunker.send(bin(1, 2, 3, 4));

    expect(result).toEqual(bin(5, 6, 7, 8));
    expect(mockIO.getWrites()).toEqual([bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [1, 2, 3, 4])]);
});

test('BleClientChunker should handle multiple chunk response', async () => {
    const mockIO = new MockIO([
        bin(1, 0, 0, 0, [5, 6]), // First chunk of the response
        bin(1, 1, 0, BleChunkHead.FINAL_CHUNK_FLAG, [7, 8]) // Final chunk
    ]);

    const chunker = new BleClientChunker(mockIO);
    const result = await chunker.send(bin(1, 2, 3, 4));

    expect(result).toEqual(bin(5, 6, 7, 8));
    expect(mockIO.getWrites()).toEqual([bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [1, 2, 3, 4])]);
});

test('BleClientChunker should retry on missed chunks with correct message id', async () => {
    const mockIO = new MockIO([
        bin(1, 0, 0, BleChunkHead.MISSED_CHUNKS_FLAG), // Simulate missed chunk
        bin(2, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [9, 10, 11, 12]) // Retry and receive correct data
    ]);

    const chunker = new BleClientChunker(mockIO);
    const result = await chunker.send(bin(1, 2, 3, 4));

    expect(result).toEqual(bin(9, 10, 11, 12));
    expect(mockIO.getWrites()).toEqual([
        bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [1, 2, 3, 4]), // Initial send
        bin(2, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [1, 2, 3, 4])  // Retry after missed chunk
    ]);
});

test('BleClientChunker should handle consecutive sends with incremented message id', async () => {
    const mockIO = new MockIO([
        bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [5, 6, 7, 8]), // First response
        bin(2, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [9, 10, 11, 12]) // Second response
    ]);

    const chunker = new BleClientChunker(mockIO);

    let result = await chunker.send(bin(1, 2, 3, 4));
    expect(result).toEqual(bin(5, 6, 7, 8));
    expect(mockIO.getWrites()[0]).toEqual(bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [1, 2, 3, 4]));

    result = await chunker.send(bin(5, 6, 7, 8));
    expect(result).toEqual(bin(9, 10, 11, 12));
    expect(mockIO.getWrites()[1]).toEqual(bin(2, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [5, 6, 7, 8]));
});

test('BleClientChunker should throw error on size overflow', async () => {
    const mockIO = new MockIO([
        bin(1, 0, 0, BleChunkHead.SIZE_OVERFLOW_FLAG) // Simulate size overflow
    ]);

    const chunker = new BleClientChunker(mockIO);

    await expect(chunker.send(bin(1, 2, 3, 4))).rejects.toThrow(/Protocol error: size overflow/);
});

test('MockIO should throw an error when no more data to read', async () => {
    const mockIO = new MockIO([bin(1, 0, 0, BleChunkHead.FINAL_CHUNK_FLAG, [5, 6, 7, 8])]);

    const chunker = new BleClientChunker(mockIO);
    await chunker.send(bin(1, 2, 3, 4)); // First read should be fine

    await expect(chunker.send(bin(1, 2, 3, 4))).rejects.toThrow(/No more data to read/);
});
