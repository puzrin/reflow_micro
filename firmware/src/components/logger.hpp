#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lib/ring_logger/ring_logger.hpp"

class GuardedLogWriter : public RingLoggerWriter<> {
public:
    explicit GuardedLogWriter(ring_logger::IRingBuffer& buf) : RingLoggerWriter<>(buf) {}
    ~GuardedLogWriter() { vSemaphoreDelete(mutex); }

protected:
    auto lock() -> void override { xSemaphoreTake(mutex, portMAX_DELAY); }
    auto unlock() -> void override { xSemaphoreGive(mutex); }

private:
    SemaphoreHandle_t mutex{xSemaphoreCreateMutex()};
};

using Logger = GuardedLogWriter;

extern Logger logger;
void logger_start();

#define DEBUG(...) logger.push(RingLoggerLevelInfo, __VA_ARGS__)
