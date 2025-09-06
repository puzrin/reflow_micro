#pragma once

#include <string>
#include "lib/async_preference.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class AsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    bool write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override;
    bool read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t max_length) override;
    size_t length(const std::string& ns, const std::string& key) override;

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
