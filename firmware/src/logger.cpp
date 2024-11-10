#include <Arduino.h>
#include "logger.hpp"

Logger logger;

char outputBuffer[1024];

static void LogOutputTask(void* pvParameters) {
    Serial.begin(115200);

    while (!Serial) vTaskDelay(pdMS_TO_TICKS(10));

    while (true) {
        while (logger.pull(outputBuffer, sizeof(outputBuffer))) {
            Serial.println(outputBuffer);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void logger_init() {
    xTaskCreate(LogOutputTask, "LogOutputTask", 1024 * 4, NULL, 1, NULL);
}