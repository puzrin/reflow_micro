#pragma once

#include <stddef.h>
#include <stdint.h>

#include <array>

#include <etl/delegate.h>
#include <etl/vector.h>

#ifndef TEST
#include "logger.hpp"
#else
#define APP_LOGE(...)
#define APP_LOGI(...)
#define APP_LOGD(...)
#endif

class BleChunkHead {
public:
    uint8_t messageId;
    uint16_t sequenceNumber;
    uint8_t flags;

    static constexpr uint8_t FINAL_CHUNK_FLAG = 0x01;
    static constexpr uint8_t MISSED_CHUNKS_FLAG = 0x02;
    static constexpr uint8_t SIZE_OVERFLOW_FLAG = 0x04;
    static constexpr size_t SIZE = 4;

    BleChunkHead(uint8_t messageId, uint16_t sequenceNumber, uint8_t flags)
        : messageId(messageId), sequenceNumber(sequenceNumber), flags(flags) {}

    explicit BleChunkHead(const uint8_t* chunk)
        : messageId(chunk[0])
        , sequenceNumber(static_cast<uint16_t>(chunk[1] | (chunk[2] << 8)))
        , flags(chunk[3]) {}

    void fillTo(uint8_t* chunk) const {
        chunk[0] = messageId;
        chunk[1] = static_cast<uint8_t>(sequenceNumber & 0xFF);
        chunk[2] = static_cast<uint8_t>((sequenceNumber >> 8) & 0xFF);
        chunk[3] = flags;
    }
};

template <size_t MaxMessageSize>
class BleChunker {
public:
    using MessageBuffer = etl::vector<uint8_t, MaxMessageSize>;
    using MessageHandler = etl::delegate<void(const MessageBuffer&, MessageBuffer&)>;

    struct ChunkView {
        const uint8_t* data;
        size_t size;
    };

    void setMessageHandler(const MessageHandler& handler) {
        onMessage = handler;
    }

    void resetMessageHandler() {
        onMessage.clear();
    }

    void consumeChunk(const uint8_t* chunk, size_t length) {
        if (length < BleChunkHead::SIZE) {
            return;
        }

        const BleChunkHead head(chunk);

        if (skipTail && head.messageId == currentMessageId) {
            // After a terminal condition for this message, discard its tail.
            return;
        }

        if (firstMessage || head.messageId != currentMessageId) {
            // New message ID resets both RX assembly and any prepared TX state.
            currentMessageId = head.messageId;
            resetState();
        }

        const size_t payload_size = length - BleChunkHead::SIZE;
        const size_t new_message_size = messageSize + payload_size;
        if (new_message_size > MaxMessageSize) {
            // Request does not fit into the fixed RX buffer.
            skipTail = true;
            prepareErrorResponse(BleChunkHead::SIZE_OVERFLOW_FLAG);
            return;
        }

        if (head.sequenceNumber != expectedSequenceNumber) {
            // Request assembly is no longer trustworthy, force a full retry.
            skipTail = true;
            prepareErrorResponse(BleChunkHead::MISSED_CHUNKS_FLAG);
            return;
        }

        assembledMessage.insert(assembledMessage.end(), chunk + BleChunkHead::SIZE, chunk + length);
        messageSize = new_message_size;
        expectedSequenceNumber++;

        if ((head.flags & BleChunkHead::FINAL_CHUNK_FLAG) == 0) {
            return;
        }

        // Full request is assembled. Keep the full response in one buffer and
        // emit BLE chunks from it lazily on reads.
        skipTail = true;
        responseMessage.clear();
        responseOffset = 0;
        responseReady = false;

        if (onMessage.is_valid()) {
            onMessage(assembledMessage, responseMessage);
        }

        responseReady = true;
    }

    auto getResponseChunk() -> ChunkView {
        if (pendingErrorFlags != 0) {
            // Transport-level errors are returned as a header-only final chunk.
            errorChunk.fill(0);
            BleChunkHead(currentMessageId, 0, pendingErrorFlags | BleChunkHead::FINAL_CHUNK_FLAG).fillTo(errorChunk.data());
            pendingErrorFlags = 0;
            return {errorChunk.data(), BleChunkHead::SIZE};
        }

        if (!responseReady) {
            // Reader may poll before the full request has been processed.
            return {noDataChunk.data(), noDataChunk.size()};
        }

        if (responseMessage.empty()) {
            // Empty response still needs a final chunk header.
            emptyChunk.fill(0);
            BleChunkHead(currentMessageId, 0, BleChunkHead::FINAL_CHUNK_FLAG).fillTo(emptyChunk.data());
            responseReady = false;
            return {emptyChunk.data(), BleChunkHead::SIZE};
        }

        if (responseOffset >= responseMessage.size()) {
            responseReady = false;
            return {noDataChunk.data(), noDataChunk.size()};
        }

        const size_t payload_size = std::min(responseMessage.size() - responseOffset, MAX_CHUNK_PAYLOAD_SIZE);
        const bool is_final = (responseOffset + payload_size) >= responseMessage.size();

        // Slice the next BLE chunk directly from the full response buffer.
        BleChunkHead(
            currentMessageId,
            static_cast<uint16_t>(responseOffset / MAX_CHUNK_PAYLOAD_SIZE),
            is_final ? BleChunkHead::FINAL_CHUNK_FLAG : 0
        ).fillTo(responseChunk.data());

        std::copy_n(responseMessage.data() + responseOffset, payload_size, responseChunk.data() + BleChunkHead::SIZE);
        responseOffset += payload_size;

        if (is_final) {
            responseReady = false;
        }

        return {responseChunk.data(), BleChunkHead::SIZE + payload_size};
    }

private:
    // Max DLE data size is 251 bytes. Every R/W to characteristic should be
    // 244 or 495 bytes to fit into 1 or 2 DLE packets (including overheads).
    // That maximizes speed on bulk transfers.
    // In real world, 495 bytes do not give any notable benefits. So, use 244 bytes.
    static constexpr size_t MAX_CHUNK_SIZE = 244;
    static constexpr size_t MAX_CHUNK_PAYLOAD_SIZE = MAX_CHUNK_SIZE - BleChunkHead::SIZE;

    uint8_t currentMessageId{0};
    size_t messageSize{0};
    uint16_t expectedSequenceNumber{0};
    bool firstMessage{true};
    bool skipTail{false};
    bool responseReady{false};
    uint8_t pendingErrorFlags{0};
    size_t responseOffset{0};

    MessageBuffer assembledMessage{};
    MessageBuffer responseMessage{};
    MessageHandler onMessage{};
    std::array<uint8_t, 1> noDataChunk{{0}};
    std::array<uint8_t, BleChunkHead::SIZE> emptyChunk{};
    std::array<uint8_t, BleChunkHead::SIZE> errorChunk{};
    std::array<uint8_t, MAX_CHUNK_SIZE> responseChunk{};

    void resetState() {
        // Reset the per-message transport state: assembled RX, prepared TX,
        // and chunk cursor/flags.
        assembledMessage.clear();
        responseMessage.clear();
        responseOffset = 0;
        responseReady = false;
        pendingErrorFlags = 0;
        messageSize = 0;
        expectedSequenceNumber = 0;
        firstMessage = false;
        skipTail = false;
    }

    void prepareErrorResponse(uint8_t error_flag) {
        // Drop any prepared response and remember the transport error to emit
        // on the next read.
        responseMessage.clear();
        responseOffset = 0;
        responseReady = false;
        pendingErrorFlags = error_flag;
    }
};
