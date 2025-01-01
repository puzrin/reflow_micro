#pragma once

#include "ring_logger_buffer.hpp"
#include "ring_logger_typelists.hpp"
#include "ring_logger_tokenizer.hpp"

constexpr int8_t RingLoggerLevelError = 0;
constexpr int8_t RingLoggerLevelInfo = 1;
constexpr int8_t RingLoggerLevelDebug = 2;

namespace ring_logger {

template<typename T>
auto decayLiteralArg(T&& x) {
    if constexpr (std::is_array_v<std::remove_reference_t<T>> &&
                  std::is_same_v<std::remove_extent_t<std::remove_reference_t<T>>, char>) {
        return static_cast<const char*>(x);
    } else {
        return std::forward<T>(x);
    }
}

} // namespace ring_logger


template <
    size_t MaxRecordSize = 512,
    typename Encoders = ring_logger::ParamEncoders_32_And_Float,
    typename Decoders = ring_logger::ParamDecoders_32_And_Float
>
class RingLogger {
public:
    explicit RingLogger(ring_logger::IRingBuffer& buf) : ringBuffer{buf} {}

    template<typename... Args>
    auto push(uint8_t level, const char* message, const Args&... msgArgs) -> void {
        using namespace ring_logger;
        ring_logger::BinVector record;

        Encoders::write(level, record);
        Encoders::write(message, record);
        (Encoders::write(decayLiteralArg(msgArgs), record), ...);

        ringBuffer.writeRecord(record);
    }

    auto pull(std::string& output) -> bool {
        using namespace ring_logger;

        ring_logger::BinVector record;
        if (!ringBuffer.readRecord(record)) { return false; }

        int32_t offset = 0;

        if (!IDecoder::isAvailableAt(record, offset)) return false;
        auto level = IDecoder::getAsNum<uint8_t>(record, offset);
        offset = IDecoder::getNextOffset(record, offset);

        if (!IDecoder::isAvailableAt(record, offset)) return false;
        auto tokenizer = StringTokenizer(IDecoder::getAsStringView(record, offset));
        offset = IDecoder::getNextOffset(record, offset);

        writeLogHeader(output, level);

        for (const auto token : tokenizer) {
            if (token.is_placeholder) {
                if (Decoders::format(record, offset, output, token.text)) {
                    offset = IDecoder::getNextOffset(record, offset);
                } else {
                    // no params left => write placeholder source
                    output.append(token.text);
                }
            } else {
                output.append(token.text);
            }
        }

        return true;
    }

    void writeLogHeader(std::string& output, uint8_t level) {
        using namespace ring_logger;

        const char* levelStr = nullptr;
        switch (level) {
            case RingLoggerLevelError: levelStr = "ERROR"; break;
            case RingLoggerLevelInfo: levelStr = "INFO"; break;
            case RingLoggerLevelDebug: levelStr = "DEBUG"; break;
            default: levelStr = "UNKNOWN"; break;
        }

        output.append("[").append(levelStr).append("]: ");
    }

private:
    ring_logger::IRingBuffer& ringBuffer;
};
