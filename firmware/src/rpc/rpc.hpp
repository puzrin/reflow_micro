#pragma once

#include "lib/msgpack_rpc_dispatcher.hpp"

extern MsgpackRpcDispatcher rpc;

void rpc_start();

void pairing_enable();
void pairing_disable();
