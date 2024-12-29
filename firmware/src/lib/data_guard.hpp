#pragma once

#include <atomic>
#include <cstdint>

// Types store with lock-free writes and optimistic reads
template <typename T>
class DataGuard {
public:
    explicit DataGuard(const T& initial = T()) : value{initial} {}

    T value;
    T snapshot{};

    void writeData(const T& newValue) {
        beginWrite();
        value = newValue;
        endWrite();
    }

    // Guards for direct modifications of value inner
    void beginWrite() { data_version.fetch_add(1, std::memory_order_release); }
    void endWrite() { data_version.fetch_add(1, std::memory_order_relaxed); }


    // Atomic clone of the value
    bool makeSnapshot() {
        const uint32_t version_before = data_version.load(std::memory_order_acquire);

        // If version is odd, it means that value is being updated right now.
        if (last_snapshot_version != version_before && version_before % 2 == 0) {
            snapshot = value;
            // If version is still the same => snapshot is useable.
            if (version_before == data_version.load(std::memory_order_acquire)) {
                last_snapshot_version = version_before;
                return true;
            }
        }

        return false;
    }

private:
    std::atomic<uint32_t> data_version{0};
    uint32_t last_snapshot_version{0};
};