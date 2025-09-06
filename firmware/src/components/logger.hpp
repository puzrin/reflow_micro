#pragma once

#include <jetlog/jetlog.hpp>

using Logger = jetlog::Writer<>;

extern Logger logger;
void logger_start();

#define APP_LOGE(...) logger.push("app", jetlog::level::error, __VA_ARGS__)
#define APP_LOGI(...) logger.push("app", jetlog::level::info, __VA_ARGS__)
#define APP_LOGD(...) logger.push("app", jetlog::level::debug, __VA_ARGS__)
