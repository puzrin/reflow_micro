#pragma once

#include <array>

#include "lib/ble_chunker.hpp"
#include "proto/generated/shared_constants.hpp"

class Session : public BleChunker<SharedConstants::MAX_RPC_MESSAGE_SIZE> {
public:
    using Base = BleChunker<SharedConstants::MAX_RPC_MESSAGE_SIZE>;
    using MessageBuffer = Base::MessageBuffer;

    Session();
    ~Session();

    void handleRpcMessage(const MessageBuffer& message, MessageBuffer& response);

    bool authenticated{false};
    std::array<uint8_t, 32> random{};
};
