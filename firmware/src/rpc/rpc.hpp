#pragma once

#include "lib/msgpack_rpc_dispatcher.hpp"

extern JsonRpcDispatcher rpc;

void rpc_init();

void pairing_enable();
void pairing_disable();
