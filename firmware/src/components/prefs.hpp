#pragma once

#include <cstdint>
#include "Preferences.h"
#include "lib/async_preference.hpp"

class AsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    auto write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) -> bool override {
        bool succeeded = prefs.begin(ns.c_str(), false);
        if (succeeded) {
            succeeded = prefs.putBytes(key.c_str(), buffer, length);
        }
        prefs.end();
        return succeeded;
    }

    auto read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t max_length) -> bool override {
        bool succeeded = prefs.begin(ns.c_str(), true);
        if (succeeded) {
            succeeded = prefs.getBytes(key.c_str(), buffer, max_length);
        }
        prefs.end();
        return succeeded;
    }

    auto length(const std::string& ns, const std::string& key) -> size_t override {
        size_t len = 0;
        if (prefs.begin(ns.c_str(), true)) {
            len = prefs.isKey(key.c_str()) ? prefs.getBytesLength(key.c_str()) : 0;
        }
        prefs.end();
        return len;
    }

    static auto getInstance() -> AsyncPreferenceKV& {
        static AsyncPreferenceKV instance;
        return instance;
    }

private:
    AsyncPreferenceKV() {} // Prohibit direct call
    Preferences prefs{};
};

class PrefsWriter : public AsyncPreferenceWriter {
public:
    void start() {
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
