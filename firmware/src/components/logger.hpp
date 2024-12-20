#pragma once

#include "lib/ring_logger/ring_logger.hpp"

using Logger = RingLogger<>;

extern Logger logger;
void logger_start();

#define DEBUG(...) logger.push_info(__VA_ARGS__)
