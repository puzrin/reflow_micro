#pragma once

#include "ring_logger/ring_logger.hpp"

using Logger = RingLogger<>;

extern Logger logger;
void logger_init();

#define DEBUG(...) logger.push_info(__VA_ARGS__)
