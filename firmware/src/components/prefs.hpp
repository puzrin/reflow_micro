#pragma once

#include <etl/string_view.h>
#include "lib/async_preference.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class AsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    bool write(etl::string_view ns, etl::string_view key, uint8_t* buffer, size_t length) override;
    bool read(etl::string_view ns, etl::string_view key, uint8_t* buffer, size_t max_length) override;
    size_t length(etl::string_view ns, etl::string_view key) override;

    static AsyncPreferenceKV& getInstance() {
        static AsyncPreferenceKV instance;
        return instance;
    }
private:
    AsyncPreferenceKV() {} // Prevent direct instantiation
};


class PrefsWriter : public AsyncPreferenceWriter {
public:
    void setup() {
        xTaskCreate([](void* arg) {
            auto* self = static_cast<PrefsWriter*>(arg);
            while(true) {
                self->tick();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }, "prefs", 1024*4, this, 0, NULL);
    }

    static auto getInstance() -> PrefsWriter& {
        static PrefsWriter instance;
        return instance;
    }

private:
    PrefsWriter() {} // Prohibit direct call
};

#define PREFS_NAMESPACE "reflow"
