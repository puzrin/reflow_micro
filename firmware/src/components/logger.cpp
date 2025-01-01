#include <Arduino.h>
#include "logger.hpp"

using namespace ring_logger;

static RingBuffer<10000> ringBuffer;
Logger logger(ringBuffer);

void logger_start() {
    xTaskCreate([](void* pvParameters) {
        (void)pvParameters;

        std::string outputBuffer;
        outputBuffer.reserve(1024);
        outputBuffer.clear();

        Serial.begin(115200);

        while (!Serial) { vTaskDelay(pdMS_TO_TICKS(10)); }

        while (true) {
            while (logger.pull(outputBuffer)) {
                Serial.println(outputBuffer.c_str());
                outputBuffer.clear();
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }, "LogOutputTask", 1024 * 4, NULL, 0, NULL);
}