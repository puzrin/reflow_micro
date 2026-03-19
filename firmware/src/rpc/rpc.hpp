#pragma once

#include <etl/string.h>

#include "lib/cbor_rpc_dispatcher.hpp"
#include "proto/generated/shared_constants.hpp"

using RpcDispatcher = cbor_rpc_dispatcher::Dispatcher<24, SharedConstants::MAX_RPC_MESSAGE_SIZE>;
using AuthRpcDispatcher = cbor_rpc_dispatcher::Dispatcher<8, SharedConstants::MAX_AUTH_RPC_MESSAGE_SIZE>;
using BleName = etl::string<SharedConstants::MAX_BLE_NAME_LENGTH + 1>;

extern RpcDispatcher rpc;
extern AuthRpcDispatcher auth_rpc;

void rpc_start();

void ble_name_write(const BleName& name);
BleName ble_name_read();
void pairing_enable();
void pairing_disable();
