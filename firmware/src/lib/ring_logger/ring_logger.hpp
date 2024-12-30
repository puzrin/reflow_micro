#pragma once

#include <vector>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <iostream>
#include "ring_logger_helpers.hpp"
#include "ring_logger_buffer.hpp"
#include "ring_logger_packer.hpp"
#include "ring_logger_formatter.hpp"

constexpr int8_t RingLoggerLevelError = 0;
constexpr int8_t RingLoggerLevelInfo = 1;
constexpr int8_t RingLoggerLevelDebug = 2;

template<size_t MaxRecordSize = 512, size_t MaxArgs = 10>
class RingLogger {
public:
    RingLogger(ring_logger::IRingBuffer &buf) : ringBuffer{buf} {}

    template<typename... Args>
    auto push(uint8_t level, const char* message, const Args&... msgArgs) -> void {
        static_assert(ring_logger::are_supported_types<typename std::decay<Args>::type...>::value, "Unsupported argument type");
        static_assert(sizeof...(msgArgs) <= MaxArgs, "Too many arguments for logging");

        uint32_t timestamp = 0;
        size_t packedSize = packer.getPackedSize(timestamp, level, message, msgArgs...);

        if (packedSize > MaxRecordSize) {
            auto packedData = packer.pack(timestamp, level, "[TOO BIG]");
            ringBuffer.writeRecord(packedData.data, packedData.size);
            return;
        }

        auto packedData = packer.pack(timestamp, level, message, msgArgs...);
        ringBuffer.writeRecord(packedData.data, packedData.size);
    }

    auto pull(char* outputBuffer, size_t bufferSize) -> bool {
        uint8_t recordData[MaxRecordSize];
        size_t recordSize = MaxRecordSize;

        if (!ringBuffer.readRecord(recordData, recordSize)) {
            return false; // No records available
        }

        typename ring_logger::Packer<MaxRecordSize, MaxArgs + 3>::UnpackedData unpackedData;
        typename ring_logger::Packer<MaxRecordSize, MaxArgs + 3>::PackedData packedData = {{0}, 0};
        std::memcpy(packedData.data, recordData, recordSize);
        packedData.size = recordSize;

        if (!packer.unpack(packedData, unpackedData)) {
            return false; // Failed to unpack
        }

        uint32_t timestamp = unpackedData.data[0].uint32Value;
        uint8_t level = unpackedData.data[1].uint8Value;
        const char* message = unpackedData.data[2].stringValue;

        size_t offset = writeLogHeader(outputBuffer, bufferSize, timestamp, level);

        ring_logger::Formatter::print(outputBuffer + offset, bufferSize - offset, message, unpackedData.data + 3, unpackedData.size - 3);

        return std::strlen(outputBuffer);
    }

private:
    ring_logger::Packer<MaxRecordSize, MaxArgs + 3> packer;
    ring_logger::IRingBuffer& ringBuffer;

    auto writeLogHeader(char* outputBuffer, size_t bufferSize, uint32_t /*timestamp*/, uint8_t level) -> size_t {
        using namespace ring_logger;

        const char* levelStr = nullptr;
        switch (level) {
            case RingLoggerLevelError: levelStr = "ERROR"; break;
            case RingLoggerLevelInfo: levelStr = "INFO"; break;
            case RingLoggerLevelDebug: levelStr = "DEBUG"; break;
            default: levelStr = "UNKNOWN"; break;
        }

        Formatter::print(outputBuffer, bufferSize, "[{}]: ", ArgVariant(levelStr));

        return std::strlen(outputBuffer);
    }
};
