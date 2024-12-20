#include "logger.hpp"
#include "rpc/rpc.hpp"
#include "components/prefs.hpp"
#include "app.hpp"

void setup() {
    logger_start();
    prefsWriter.start();

    app.setup();

    // Demo methods
    rpc.addMethod("echo", [](const std::string msg)-> std::string { return msg; });
    rpc.addMethod("devnull", [](const std::string msg)-> bool { return true; });
    rpc.addMethod("bintest", [](const std::vector<uint8_t> data) -> std::vector<uint8_t> {
        std::vector<uint8_t> result;
        for(uint8_t byte : data) result.push_back(byte + 3);
        return result;
    });

    rpc_init();
}

void loop() {}