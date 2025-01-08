#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rom/ets_sys.h"
#include "logger.hpp"
#include "hal/usb_serial_jtag_ll.h"

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

        // Wait until usb serial ready, or startup messages will be lost
        while(!usb_serial_jtag_ll_txfifo_writable()) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        while (true) {
            while (logReader.pull(outputBuffer)) {
                ets_printf("%s\n", outputBuffer.c_str());
                outputBuffer.clear();
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }, "LogOutputTask", 1024 * 4, NULL, 0, NULL);
}