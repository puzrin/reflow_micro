#include "logger.hpp"
#include "blinker/blinker.h"
#include "button/button.hpp"
#include "rpc/rpc.hpp"
#include "async_preference/prefs.hpp"
#include "app.hpp"

void setup() {
    logger_init();
    prefs_init();
    blinker_init();
    button_init();

    // Demo methods
    rpc.addMethod("echo", [](const std::string msg)-> std::string { return msg; });
    rpc.addMethod("devnull", [](const std::string msg)-> bool { return true; });

    rpc_init();
    app_init();
}

void loop() {}