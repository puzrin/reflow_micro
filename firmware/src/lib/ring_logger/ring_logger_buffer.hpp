#pragma once

#include <atomic>
#include <array>
#include <algorithm>  // For std::copy_n

namespace ring_logger {

class IRingBuffer {
public:
    virtual auto writeRecord(const uint8_t* data, size_t size) -> bool = 0;
    virtual auto readRecord(uint8_t* data, size_t& size) -> bool = 0;
};

template <size_t BufferSize>
class RingBuffer : public IRingBuffer {
public:
    struct RecordHeader {
        uint16_t size;
    };

    auto writeRecord(const uint8_t* data, size_t size) -> bool override {
        size_t record_size = sizeof(RecordHeader) + size; // Include size header
        size_t head, next_head;

        do {
            head = head_idx.load(std::memory_order_acquire);
            next_head = (head + record_size) % BufferSize;

            // Ensure enough space is available
            freeSpace(record_size);

            // Re-check head to ensure space is still valid
        } while (!head_idx.compare_exchange_weak(head, next_head, std::memory_order_release, std::memory_order_relaxed));

        // Write record header (size)
        RecordHeader header = { static_cast<uint16_t>(size) };
        setRecordHeader(head, header);

        // Write record data
        writeBuffer((head + sizeof(RecordHeader)) % BufferSize, data, size);

        return true;
    }

    auto readRecord(uint8_t* data, size_t& size) -> bool override {
        while (true) {
            size_t tail = tail_idx.load(std::memory_order_acquire);
            size_t head = head_idx.load(std::memory_order_acquire);

            // No data records
            if (tail == head) {
                size = 0;
                return false;
            }

            // Read record header (size)
            RecordHeader header;
            getRecordHeader(tail, header);
            size = header.size;

            // Make sure data was not corrupted, tail should not be changed.
            // Start from the beginning on fail.
            if (tail_idx.load(std::memory_order_acquire) != tail) { continue; }

            size_t next_tail = (tail + sizeof(RecordHeader) + size) % BufferSize;

            // Extract record data. It can become corrupted,
            // so checked in the next step.
            readBuffer((tail + sizeof(RecordHeader)) % BufferSize, data, size);

            // Check tail was not changed from outside, update and finish on success
            if (tail_idx.compare_exchange_weak(tail, next_tail, std::memory_order_release, std::memory_order_relaxed)) { break; }
        }

        return true;
    }

private:
    void freeSpace(size_t required_space) {
        while (true) {
            size_t head = head_idx.load(std::memory_order_acquire);
            size_t tail = tail_idx.load(std::memory_order_acquire);

            size_t space_available = head >= tail ? (BufferSize - head + tail) : (tail - head);

            // Exit if enough space
            if (space_available >= required_space) { return; }

            // Release a single record
            // Content can be invalid at this moment if tail changed.
            // But this is safe for calculation and bad new_tail value will not be stored.
            RecordHeader header;
            getRecordHeader(tail, header);
            size_t new_tail = (tail + sizeof(RecordHeader) + header.size) % BufferSize;

            // Update tail pointer atomically
            tail_idx.compare_exchange_weak(tail, new_tail, std::memory_order_release, std::memory_order_relaxed);
        }
    }

    inline void getRecordHeader(size_t index, RecordHeader& header) const {
        if (index + sizeof(RecordHeader) <= BufferSize) {
            std::copy_n(&buffer[index], sizeof(RecordHeader), reinterpret_cast<uint8_t*>(&header));
        } else {
            size_t first_part = BufferSize - index;
            std::copy_n(&buffer[index], first_part, reinterpret_cast<uint8_t*>(&header));
            std::copy_n(&buffer[0], sizeof(RecordHeader) - first_part,
                       reinterpret_cast<uint8_t*>(&header) + first_part);
        }
    }

    inline void setRecordHeader(size_t index, const RecordHeader& header) {
        if (index + sizeof(RecordHeader) <= BufferSize) {
            std::copy_n(reinterpret_cast<const uint8_t*>(&header), sizeof(RecordHeader), &buffer[index]);
        } else {
            size_t first_part = BufferSize - index;
            std::copy_n(reinterpret_cast<const uint8_t*>(&header), first_part, &buffer[index]);
            std::copy_n(reinterpret_cast<const uint8_t*>(&header) + first_part,
                       sizeof(RecordHeader) - first_part, &buffer[0]);
        }
    }

    inline void writeBuffer(size_t index, const uint8_t* data, size_t size) {
        if (index + size <= BufferSize) {
            std::copy_n(data, size, &buffer[index]);
        } else {
            size_t first_part = BufferSize - index;
            std::copy_n(data, first_part, &buffer[index]);
            std::copy_n(data + first_part, size - first_part, &buffer[0]);
        }
    }

    inline void readBuffer(size_t index, uint8_t* data, size_t size) const {
        if (index + size <= BufferSize) {
            std::copy_n(&buffer[index], size, data);
        } else {
            size_t first_part = BufferSize - index;
            std::copy_n(&buffer[index], first_part, data);
            std::copy_n(&buffer[0], size - first_part, data + first_part);
        }
    }

    std::array<uint8_t, BufferSize> buffer{};
    std::atomic<size_t> head_idx{0};
    std::atomic<size_t> tail_idx{0};
};

} // namespace ring_logger