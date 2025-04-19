#pragma once

#include "jetlog/jetlog.hpp"

using Logger = jetlog::Writer<>;

extern Logger logger;
void logger_start();

#define DEBUG(...) logger.push("app", jetlog::level::info, __VA_ARGS__)
