#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rom/ets_sys.h"
#include "logger.hpp"
#include "hal/usb_serial_jtag_ll.h"

static jetlog::RingBuffer<10000> ringBuffer;

Logger logger(ringBuffer);
LogReader logReader(ringBuffer);

auto Logger::getTime() -> uint32_t {
    return Time::now();
}

void LogReader::writeLogHeader(etl::istring& output, uint32_t timestamp, const etl::string_view& tag, uint8_t level) {
    // Add log level
    output.append(level2str(level));

    // Format timestamp as seconds.milliseconds
    if (timestamp != etl::numeric_limits<uint32_t>::max()) {
        output.append(" (");

        uint32_t seconds = timestamp / 1000;
        uint32_t millis = timestamp % 1000;

        etl::to_string(seconds, output, true);
        output.append(".");

        // Add leading zeros for milliseconds (e.g., 001, 023, 456)
        if (millis < 100) output.append("0");
        if (millis < 10) output.append("0");
        etl::to_string(millis, output, true);

        output.append(")");
    }

    // Add tag
    if (tag.length() > 0) {
        output.append(" ");
        output.append(tag.begin(), tag.end());
    }

    output.append(": ");
}

void logger_start() {
    xTaskCreate([](void* /*pvParameters*/) {
        etl::string<1024> outputBuffer{};

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