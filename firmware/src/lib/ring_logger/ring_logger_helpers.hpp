#pragma once

#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace ring_logger {

    enum class ArgTypeTag : uint8_t {
        INT8, INT16, INT32, UINT8, UINT16, UINT32, STRING
    };

    struct ArgVariant {
        ArgTypeTag type;
        union {
            int8_t int8Value;
            int16_t int16Value;
            int32_t int32Value;
            uint8_t uint8Value;
            uint16_t uint16Value;
            uint32_t uint32Value;
            const char* stringValue;
        };

        ArgVariant() : type(ArgTypeTag::INT8), int8Value(0) {}
        explicit ArgVariant(int8_t value) : type(ArgTypeTag::INT8), int8Value(value) {}
        explicit ArgVariant(int16_t value) : type(ArgTypeTag::INT16), int16Value(value) {}
        explicit ArgVariant(int32_t value) : type(ArgTypeTag::INT32), int32Value(value) {}
        explicit ArgVariant(uint8_t value) : type(ArgTypeTag::UINT8), uint8Value(value) {}
        explicit ArgVariant(uint16_t value) : type(ArgTypeTag::UINT16), uint16Value(value) {}
        explicit ArgVariant(uint32_t value) : type(ArgTypeTag::UINT32), uint32Value(value) {}
        explicit ArgVariant(const char* value) : type(ArgTypeTag::STRING), stringValue(value) {}
    };

    template<typename T>
    struct is_diverged_int : std::integral_constant<bool, std::is_same<T, int>::value && !std::is_same<int, int32_t>::value && sizeof(int) == sizeof(int32_t)> {};

    // On some platform like RISC-V "int" may be != any of strict intXX_t types.
    // We allow such diverged int-s as "supported type".
    // If `int` not diverged (same as one of intXX_t), it's not added to avoid double definition.
    // Also this is false by default for other non-specialized types.
    template<typename T>
    struct is_supported_type
        : std::conditional<
            is_diverged_int<T>::value,
            std::true_type,
            std::false_type
        >::type {};

    // Explicit specializations for supported types
    template<> struct is_supported_type<int8_t> : std::true_type {};
    template<> struct is_supported_type<uint8_t> : std::true_type {};
    template<> struct is_supported_type<int16_t> : std::true_type {};
    template<> struct is_supported_type<uint16_t> : std::true_type {};
    template<> struct is_supported_type<int32_t> : std::true_type {};
    template<> struct is_supported_type<uint32_t> : std::true_type {};
    template<> struct is_supported_type<const char*> : std::true_type {};
    template<> struct is_supported_type<char*> : std::true_type {};

    template<typename... Args>
    struct are_supported_types;

    template<>
    struct are_supported_types<> : std::true_type {};

    template<typename T, typename... Rest>
    struct are_supported_types<T, Rest...>
        : std::integral_constant<bool, is_supported_type<typename std::decay<T>::type>::value && are_supported_types<Rest...>::value> {};

} // namespace ring_logger
