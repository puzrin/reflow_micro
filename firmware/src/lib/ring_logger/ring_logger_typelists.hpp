#pragma once

#include "ring_logger_types.hpp"

namespace ring_logger {

template<template<typename> class... Es>
struct EncoderList {
    template<typename T>
    static constexpr bool has_matching = (Es<T>::matchType || ...);

    template<typename T>
    static void write(const T& value, BinVector& out) {
        static_assert(has_matching<T>, "No matching encoder found");
        ([&]() {
            if constexpr (Es<T>::matchType) {
                Es<T>::write(value, out);
                return true;
            }
            return false;
        }() || ...);
    }
};

template<typename... Ds>
struct DecoderList {
    static bool format(const BinVector& data, size_t offset, std::string& output, std::string_view fmt = {}) {
        // First check if data is available
        if (!IDecoder::isAvailableAt(data, offset)) {
            return false;
        }

        bool decoded = ([&]() {
            if (Ds::matchTypeTag(IDecoder::pickTypeID(data, offset))) {
                Ds(data, offset).format(output, fmt);
                return true;
            }
            return false;
        }() || ...);

        if (!decoded) {
            DecoderUnknown(data, offset).format(output, fmt);
        }

        return true;
    }
};

//
// Several pre-defined list variants for quick-choose
//

using ParamEncoders_32_No_Float = EncoderList<
    EncoderI8, EncoderU8, EncoderI16, EncoderU16, EncoderI32, EncoderU32,
    EncoderStdString, EncoderCString
>;

using ParamDecoders_32_No_Float = DecoderList<
    DecoderI8, DecoderU8, DecoderI16, DecoderU16, DecoderI32, DecoderU32,
    DecoderStr
>;

using ParamEncoders_32_And_Float = EncoderList<
    EncoderI8, EncoderU8, EncoderI16, EncoderU16, EncoderI32, EncoderU32,
    EncoderStdString, EncoderCString,
    EncoderFlt
>;

using ParamDecoders_32_And_Float = DecoderList<
    DecoderI8, DecoderU8, DecoderI16, DecoderU16, DecoderI32, DecoderU32,
    DecoderStr,
    DecoderFlt
>;

using ParamEncoders_64_And_Double = EncoderList<
    EncoderI8, EncoderU8, EncoderI16, EncoderU16, EncoderI32, EncoderU32,
    EncoderStdString, EncoderCString,
    EncoderI64, EncoderU64,
    EncoderFlt,
    EncoderDbl
>;

using ParamDecoders_64_And_Double = DecoderList<
    DecoderI8, DecoderU8, DecoderI16, DecoderU16, DecoderI32, DecoderU32,
    DecoderStr,
    DecoderI64, DecoderU64,
    DecoderFlt,
    DecoderDbl
>;

} // namespace ring_logger
