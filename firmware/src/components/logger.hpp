#pragma once

#include <jetlog/jetlog.hpp>

using LogWriter = jetlog::Writer<>;

class LogReader : public jetlog::Reader<> {
public:
    LogReader(jetlog::IRingBuffer& buf) : jetlog::Reader<>(buf) {}
    void writeLogHeader(etl::istring& output, uint32_t timestamp, const etl::string_view& tag, uint8_t level) override;
};

extern LogWriter logger;
void logger_start();

#define APP_LOGE(...) logger.push("app", jetlog::level::error, __VA_ARGS__)
#define APP_LOGI(...) logger.push("app", jetlog::level::info, __VA_ARGS__)
#define APP_LOGD(...) logger.push("app", jetlog::level::debug, __VA_ARGS__)
#define APP_LOGV(...) logger.push("app", jetlog::level::verbose, __VA_ARGS__)
