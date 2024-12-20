#pragma once

#include <atomic>
#include <cstring> // For std::memcpy

namespace ring_logger {

template <size_t BufferSize>
class RingBuffer {
public:
    struct RecordHeader {
        uint16_t size;
    };

    RingBuffer() : head(0), tail(0) {}

    bool writeRecord(const uint8_t* data, size_t size) {
        size_t record_size = sizeof(RecordHeader) + size; // Include size header
        size_t head, next_head;

        do {
            head = this->head.load(std::memory_order_acquire);
            next_head = (head + record_size) % BufferSize;

            // Ensure enough space is available
            this->freeSpace(record_size);

            // Re-check head to ensure space is still valid
        } while (!this->head.compare_exchange_weak(head, next_head, std::memory_order_release, std::memory_order_relaxed));

        // Write record header (size)
        RecordHeader header = { static_cast<uint16_t>(size) };
        setRecordHeader(head, header);

        // Write record data
        writeBuffer((head + sizeof(RecordHeader)) % BufferSize, data, size);

        return true;
    }

    bool readRecord(uint8_t* data, size_t& size) {
        size_t tail, head, next_tail;

        while (true) {
            tail = this->tail.load(std::memory_order_acquire);
            head = this->head.load(std::memory_order_acquire);

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
            if (this->tail.load(std::memory_order_acquire) != tail) continue;

            next_tail = (tail + sizeof(RecordHeader) + size) % BufferSize;

            // Extract record data. It can become corrupted,
            // so checked in the next step.
            readBuffer((tail + sizeof(RecordHeader)) % BufferSize, data, size);

            // Check tail was not changed from outside, update and finish on success
            if (this->tail.compare_exchange_weak(tail, next_tail, std::memory_order_release, std::memory_order_relaxed)) break;
        }

        return true;
    }

private:
    void freeSpace(size_t required_space) {
        size_t tail, head, new_tail;

        while (true) {
            head = this->head.load(std::memory_order_acquire);
            tail = this->tail.load(std::memory_order_acquire);

            size_t space_available = head >= tail ? (BufferSize - head + tail) : (tail - head);

            // Exit if enough space
            if (space_available >= required_space) return;

            // Release a single record
            // Content can be invalid at this moment if tail changed.
            // But this is safe for calculation and bad new_tail value will not be stored.
            RecordHeader header;
            getRecordHeader(tail, header);
            new_tail = (tail + sizeof(RecordHeader) + header.size) % BufferSize;

            // Update tail pointer atomically
            this->tail.compare_exchange_weak(tail, new_tail, std::memory_order_release, std::memory_order_relaxed);
        }
    }

    inline void getRecordHeader(size_t index, RecordHeader& header) const {
        if (index + sizeof(RecordHeader) <= BufferSize) {
            std::memcpy(&header, &this->buffer[index], sizeof(RecordHeader));
        } else {
            size_t first_part = BufferSize - index;
            std::memcpy(&header, &this->buffer[index], first_part);
            std::memcpy(reinterpret_cast<char*>(&header) + first_part, &this->buffer[0], sizeof(RecordHeader) - first_part);
        }
    }

    inline void setRecordHeader(size_t index, const RecordHeader& header) {
        if (index + sizeof(RecordHeader) <= BufferSize) {
            std::memcpy(&this->buffer[index], &header, sizeof(RecordHeader));
        } else {
            size_t first_part = BufferSize - index;
            std::memcpy(&this->buffer[index], &header, first_part);
            std::memcpy(&this->buffer[0], reinterpret_cast<const char*>(&header) + first_part, sizeof(RecordHeader) - first_part);
        }
    }

    inline void writeBuffer(size_t index, const uint8_t* data, size_t size) {
        if (index + size <= BufferSize) {
            std::memcpy(&this->buffer[index], data, size);
        } else {
            size_t first_part = BufferSize - index;
            std::memcpy(&this->buffer[index], data, first_part);
            std::memcpy(&this->buffer[0], data + first_part, size - first_part);
        }
    }

    inline void readBuffer(size_t index, uint8_t* data, size_t size) const {
        if (index + size <= BufferSize) {
            std::memcpy(data, &this->buffer[index], size);
        } else {
            size_t first_part = BufferSize - index;
            std::memcpy(data, &this->buffer[index], first_part);
            std::memcpy(data + first_part, &this->buffer[0], size - first_part);
        }
    }

    uint8_t buffer[BufferSize];
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
};

} // namespace ring_logger
