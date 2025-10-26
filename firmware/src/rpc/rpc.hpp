#pragma once

#include "lib/msgpack_rpc_dispatcher.hpp"

extern MsgpackRpcDispatcher rpc;

void rpc_start();

void ble_name_write(const std::string& name);
std::string ble_name_read();
void pairing_enable();
void pairing_disable();
