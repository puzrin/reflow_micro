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

enum class RingLoggerLevel {
    DEBUG,
    INFO,
    ERROR,
    NONE // Highest level to disable all logs
};

namespace ring_logger {

    template<RingLoggerLevel level, const char* label, RingLoggerLevel CompileTimeLogLevel, const char* allowedLabels, const char* ignoredLabels>
    struct should_log {
        static const bool value = level >= CompileTimeLogLevel &&
                                  (allowedLabels == nullptr || is_label_in_list(label, allowedLabels)) &&
                                  (ignoredLabels == nullptr || !is_label_in_list(label, ignoredLabels));
    };

} // namespace ring_logger

// WARNING: if you decide use allowedLabels/ignoredLabels features, those MUST
// be declared INLINE constexpr to use log in multiple files, otherwise you will
// get linker errors. This requires c++17 or later. For older version - keep
// allowedLabels/ignoredLabels as nullptr and use only CompileTimeLogLevel.

template<
    size_t BufferSize = 10 * 1024,
    RingLoggerLevel CompileTimeLogLevel = RingLoggerLevel::DEBUG,
    size_t MaxRecordSize = 512,
    size_t MaxArgs = 10,
    const char* AllowedLabels = nullptr,
    const char* IgnoredLabels = nullptr
>
class RingLogger {
public:
    RingLogger() {}

    template<RingLoggerLevel level, typename... Args>
    void push(const char* message, const Args&... msgArgs) {
        lpush<level, nullptr>(message, msgArgs...);
    }

    template<RingLoggerLevel level, const char* label, typename... Args>
    typename std::enable_if<ring_logger::should_log<level, label, CompileTimeLogLevel, AllowedLabels, IgnoredLabels>::value>::type
    lpush(const char* message, const Args&... msgArgs) {
        static_assert(ring_logger::are_supported_types<typename std::decay<Args>::type...>::value, "Unsupported argument type");
        static_assert(level != RingLoggerLevel::NONE, "NONE log level is invalid for logging");
        static_assert(label == nullptr || label[0] != ' ', "Label should not start with a space");
        static_assert(label == nullptr || label[0] == '\0' || label[std::strlen(label ? label : "") - 1] != ' ', "Label should not end with a space");
        static_assert(sizeof...(msgArgs) <= MaxArgs, "Too many arguments for logging");

        uint32_t timestamp = 0;
        uint8_t level_as_byte = static_cast<uint8_t>(level);
        const char* safe_label = (label == nullptr) ? "" : label;
        size_t packedSize = packer.getPackedSize(timestamp, level_as_byte, safe_label, message, msgArgs...);

        if (packedSize > MaxRecordSize) {
            auto packedData = packer.pack(timestamp, level_as_byte, safe_label, "[TOO BIG]");
            ringBuffer.writeRecord(packedData.data, packedData.size);
            return;
        }

        auto packedData = packer.pack(timestamp, level_as_byte, safe_label, message, msgArgs...);
        ringBuffer.writeRecord(packedData.data, packedData.size);
    }

    template<RingLoggerLevel level, const char* label, typename... Args>
    typename std::enable_if<!ring_logger::should_log<level, label, CompileTimeLogLevel, AllowedLabels, IgnoredLabels>::value>::type
    lpush(const char* /*message*/, const Args&... /*msgArgs*/) {
        // Empty implementation for disabled conditions
    }

    bool pull(char* outputBuffer, size_t bufferSize) {
        uint8_t recordData[MaxRecordSize];
        size_t recordSize = MaxRecordSize;

        if (!ringBuffer.readRecord(recordData, recordSize)) {
            return false; // No records available
        }

        typename ring_logger::Packer<MaxRecordSize, MaxArgs + 4>::UnpackedData unpackedData;
        typename ring_logger::Packer<MaxRecordSize, MaxArgs + 4>::PackedData packedData = {{0}, 0};
        std::memcpy(packedData.data, recordData, recordSize);
        packedData.size = recordSize;

        if (!packer.unpack(packedData, unpackedData)) {
            return false; // Failed to unpack
        }

        uint32_t timestamp = unpackedData.data[0].uint32Value;
        uint8_t level_as_byte = unpackedData.data[1].uint8Value;
        RingLoggerLevel level = static_cast<RingLoggerLevel>(level_as_byte);
        const char* label = unpackedData.data[2].stringValue;
        const char* message = unpackedData.data[3].stringValue;

        size_t offset = writeLogHeader(outputBuffer, bufferSize, timestamp, level, label);

        ring_logger::Formatter::print(outputBuffer + offset, bufferSize - offset, message, unpackedData.data + 4, unpackedData.size - 4);

        return std::strlen(outputBuffer);
    }

    template<typename... Args>
    void push_info(const char* message, const Args&... msgArgs) {
        push<RingLoggerLevel::INFO>(message, msgArgs...);
    }

    template<const char* label, typename... Args>
    void lpush_info(const char* message, const Args&... msgArgs) {
        lpush<RingLoggerLevel::INFO, label>(message, msgArgs...);
    }

    template<typename... Args>
    void push_debug(const char* message, const Args&... msgArgs) {
        push<RingLoggerLevel::DEBUG>(message, msgArgs...);
    }

    template<const char* label, typename... Args>
    void lpush_debug(const char* message, const Args&... msgArgs) {
        lpush<RingLoggerLevel::DEBUG, label>(message, msgArgs...);
    }

    template<typename... Args>
    void push_error(const char* message, const Args&... msgArgs) {
        push<RingLoggerLevel::ERROR>(message, msgArgs...);
    }

    template<const char* label, typename... Args>
    void lpush_error(const char* message, const Args&... msgArgs) {
        lpush<RingLoggerLevel::ERROR, label>(message, msgArgs...);
    }

private:
    ring_logger::Packer<MaxRecordSize, MaxArgs + 4> packer;
    ring_logger::RingBuffer<BufferSize> ringBuffer;

    size_t writeLogHeader(char* outputBuffer, size_t bufferSize, uint32_t /*timestamp*/, RingLoggerLevel level, const char* label) {
        using namespace ring_logger;

        const char* levelStr = nullptr;
        switch (level) {
            case RingLoggerLevel::DEBUG: levelStr = "DEBUG"; break;
            case RingLoggerLevel::INFO: levelStr = "INFO"; break;
            case RingLoggerLevel::ERROR: levelStr = "ERROR"; break;
            case RingLoggerLevel::NONE: levelStr = "NONE"; break;
            default: levelStr = "UNKNOWN"; break;
        }

        if (label[0] == '\0') {
            Formatter::print(outputBuffer, bufferSize, "[{}]: ", ArgVariant(levelStr));
        } else {
            Formatter::print(outputBuffer, bufferSize, "[{}] [{}]: ", ArgVariant(levelStr), ArgVariant(label));
        }

        return std::strlen(outputBuffer);
    }
};
