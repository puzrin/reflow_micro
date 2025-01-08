#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_rom_sys.h"
#include "logger.hpp"

using namespace ring_logger;

static RingBuffer<10000> ringBuffer;
Logger logger(ringBuffer);
RingLoggerReader<> logReader(ringBuffer);

void logger_start() {
    xTaskCreate([](void* pvParameters) {
        (void)pvParameters;

        std::string outputBuffer;
        outputBuffer.reserve(1024);
        outputBuffer.clear();

        // Wait for JTAG to be ready. This is a workaround for the issue when
        // the first messages are lost.
        esp_rom_delay_us(100 * 1000);

        while (true) {
            while (logReader.pull(outputBuffer)) {
                ets_printf("%s\n", outputBuffer.c_str());
                outputBuffer.clear();
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }, "LogOutputTask", 1024 * 4, NULL, 0, NULL);
}