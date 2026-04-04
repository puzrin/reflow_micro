#pragma once

#include <array>
#include <stdint.h>
#include <host/ble_gap.h>

#include "lib/ble_chunker.hpp"
#include "proto/generated/shared_constants.hpp"

class Session : public BleChunker<SharedConstants::MAX_RPC_MESSAGE_SIZE> {
public:
    using Base = BleChunker<SharedConstants::MAX_RPC_MESSAGE_SIZE>;
    using MessageBuffer = Base::MessageBuffer;
    using ConnHandle = decltype(ble_gap_conn_desc::conn_handle);

    explicit Session(ConnHandle conn_handle);
    void handleRpcMessage(const MessageBuffer& message, MessageBuffer& response);

    ConnHandle conn_handle;
    bool authenticated{false};
    std::array<uint8_t, 32> random{};
};
