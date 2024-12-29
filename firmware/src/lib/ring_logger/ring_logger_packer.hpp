#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "ring_logger_helpers.hpp"

namespace ring_logger {

template<size_t MAX_BUFFER_SIZE, size_t MAX_ARGUMENTS>
class Packer {
public:
    struct PackedData {
        uint8_t data[MAX_BUFFER_SIZE];
        size_t size; // Actual size of the packed data
    };

    struct UnpackedData {
        ArgVariant data[MAX_ARGUMENTS];
        size_t size; // Actual number of unpacked arguments
    };

    template<typename... Args>
    static auto pack(const Args&... args) -> PackedData {
        static_assert(sizeof...(args) <= MAX_ARGUMENTS, "Number of arguments exceeds the maximum allowed");

        PackedData packedData = {};
        size_t offset = 0;

        // Write the number of arguments
        packedData.data[offset++] = sizeof...(args);

        // Serialize each argument
        int dummy[] = { 0, (serializeArgument(packedData.data, offset, args), 0)... };
        static_cast<void>(dummy); // Avoid unused variable warning

        packedData.size = offset;
        return packedData;
    }

    static auto unpack(const PackedData& packedData, UnpackedData& unpackedData) -> bool {
        size_t offset = 0;

        // Read the number of arguments
        unpackedData.size = packedData.data[offset++];
        if (unpackedData.size > MAX_ARGUMENTS) {
            return false; // More arguments than allowed
        }

        for (size_t i = 0; i < unpackedData.size; ++i) {
            ArgTypeTag type = static_cast<ArgTypeTag>(packedData.data[offset++]);
            switch (type) {
                case ArgTypeTag::INT8:
                    unpackedData.data[i] = ArgVariant(deserialize<int8_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::INT16:
                    unpackedData.data[i] = ArgVariant(deserialize<int16_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::INT32:
                    unpackedData.data[i] = ArgVariant(deserialize<int32_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::UINT8:
                    unpackedData.data[i] = ArgVariant(deserialize<uint8_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::UINT16:
                    unpackedData.data[i] = ArgVariant(deserialize<uint16_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::UINT32:
                    unpackedData.data[i] = ArgVariant(deserialize<uint32_t>(packedData.data, offset));
                    break;
                case ArgTypeTag::STRING:
                    unpackedData.data[i] = ArgVariant(deserializeString(packedData.data, offset));
                    break;
                default:
                    return false; // Unknown data type
            }
        }
        return true;
    }

    template<typename... Args>
    static auto getPackedSize(const Args&... args) -> size_t {
        size_t size = 1; // For the number of arguments
        int dummy[] = { 0, (calculateArgumentSize(size, args), 0)... };
        static_cast<void>(dummy); // Avoid unused variable warning
        return size;
    }

private:
    template<typename T>
    static void serializeArgument(uint8_t* buffer, size_t& offset, const T& value) {
        serialize(buffer, offset, value);
    }

    template<typename T>
    static void serialize(uint8_t* buffer, size_t& offset, const T& value, ArgTypeTag type) {
        buffer[offset++] = static_cast<uint8_t>(type);
        std::memcpy(buffer + offset, &value, sizeof(value));
        offset += sizeof(value);
    }

    static void serialize(uint8_t* buffer, size_t& offset, int8_t value) {
        serialize(buffer, offset, value, ArgTypeTag::INT8);
    }

    static void serialize(uint8_t* buffer, size_t& offset, int16_t value) {
        serialize(buffer, offset, value, ArgTypeTag::INT16);
    }

    static void serialize(uint8_t* buffer, size_t& offset, int32_t value) {
        serialize(buffer, offset, value, ArgTypeTag::INT32);
    }

    static void serialize(uint8_t* buffer, size_t& offset, uint8_t value) {
        serialize(buffer, offset, value, ArgTypeTag::UINT8);
    }

    static void serialize(uint8_t* buffer, size_t& offset, uint16_t value) {
        serialize(buffer, offset, value, ArgTypeTag::UINT16);
    }

    static void serialize(uint8_t* buffer, size_t& offset, uint32_t value) {
        serialize(buffer, offset, value, ArgTypeTag::UINT32);
    }

    static void serialize(uint8_t* buffer, size_t& offset, const char* value) {
        buffer[offset++] = static_cast<uint8_t>(ArgTypeTag::STRING);
        size_t length = std::strlen(value) + 1; // Include trailing zero
        uint16_t length16 = static_cast<uint16_t>(length);
        std::memcpy(buffer + offset, &length16, sizeof(length16));
        offset += sizeof(length16);
        std::memcpy(buffer + offset, value, length);
        offset += length;
    }

    // SFINAE serialize for diverged int
    template<typename T>
    static typename std::enable_if<is_diverged_int<T>::value>::type
    serialize(uint8_t* buffer, size_t& offset, T value) {
        serialize(buffer, offset, static_cast<int32_t>(value));
    }

    template<typename T>
    static auto deserialize(const uint8_t* buffer, size_t& offset) -> T {
        T value;
        std::memcpy(&value, buffer + offset, sizeof(value));
        offset += sizeof(value);
        return value;
    }

    static auto deserializeString(const uint8_t* buffer, size_t& offset) -> const char* {
        uint16_t length;
        std::memcpy(&length, buffer + offset, sizeof(length));
        offset += sizeof(length);
        const char* str = reinterpret_cast<const char*>(buffer + offset);
        offset += length;
        return str;
    }

    template<typename T>
    static void calculateArgumentSize(size_t& size, const T& value) {
        size += sizeof(ArgTypeTag) + sizeof(value);
    }

    static void calculateArgumentSize(size_t& size, const char* value) {
        size += sizeof(ArgTypeTag) + sizeof(uint16_t) + std::strlen(value) + 1; // Include trailing zero
    }
};

} // namespace ring_logger
