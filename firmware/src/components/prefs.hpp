#pragma once

#include <cstdint>
#include "Preferences.h"
#include "lib/async_preference.hpp"

class AsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    bool write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        bool succeeded = prefs.begin(ns.c_str(), false);
        if (succeeded) {
            succeeded = prefs.putBytes(key.c_str(), buffer, length);
        }
        prefs.end();
        return succeeded;
    }

    bool read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t max_length) override {
        bool succeeded = prefs.begin(ns.c_str(), true);
        if (succeeded) {
            succeeded = prefs.getBytes(key.c_str(), buffer, max_length);
        }
        prefs.end();
        return succeeded;
    }

    size_t length(const std::string& ns, const std::string& key) override {
        size_t len = 0;
        if (prefs.begin(ns.c_str(), true)) {
            len = prefs.isKey(key.c_str()) ? prefs.getBytesLength(key.c_str()) : 0;
        }
        prefs.end();
        return len;
    }

    static AsyncPreferenceKV& getInstance() {
        static AsyncPreferenceKV instance;
        return instance;
    }

private:
    AsyncPreferenceKV() {} // Prohibit direct call
    Preferences prefs;
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

    static PrefsWriter& getInstance() {
        static PrefsWriter instance;
        return instance;
    }

private:
    PrefsWriter() {} // Prohibit direct call
};

#define PREFS_NAMESPACE "reflow"
