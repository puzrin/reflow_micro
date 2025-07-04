#include "app.hpp"
#include "components/prefs.hpp"
#include "components/stack_monitor.hpp"
#include "logger.hpp"
#include "rpc/rpc.hpp"

extern "C" {
    void app_main(void);
}

extern "C" void app_main() {
    // Temporary. Force heater off until control implemented.
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_20),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_20, 0);

    logger_start();
    PrefsWriter::getInstance().start();

    application.setup();

    // Demo methods
    rpc.addMethod("echo", [](const std::string msg)-> std::string { return msg; });
    rpc.addMethod("bintest", [](const std::vector<uint8_t> data) -> std::vector<uint8_t> {
        std::vector<uint8_t> result;
        for(uint8_t byte : data) result.push_back(static_cast<uint8_t>(byte + 3));
        return result;
    });
    rpc.addMethod("devnull", [](const std::vector<uint8_t> msg)-> bool { return true; });
    rpc.addMethod("get16K", []() -> std::vector<uint8_t> {
        std::vector<uint8_t> result(1024*16, 0xF0);
        return result;
    });

    rpc_start();

    stack_monitor_start();
}
