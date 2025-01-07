#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lib/ring_logger/ring_logger.hpp"

using Logger = RingLoggerWriter<>;

extern Logger logger;
void logger_start();

#define DEBUG(...) logger.push(RingLoggerLevelInfo, __VA_ARGS__)
