#pragma once

#include <cstdint>
#include "Preferences.h"
#include "lib/async_preference.hpp"

class AsyncPreferenceKV : public IAsyncPreferenceKV {
    Preferences prefs;

    void write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        prefs.begin(ns.c_str(), false);
        prefs.putBytes(key.c_str(), buffer, length);
        prefs.end();
    }

    void read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        prefs.begin(ns.c_str(), true);
        prefs.getBytes(key.c_str(), buffer, length);
        prefs.end();
    }

    size_t length(const std::string& ns, const std::string& key) override {
        prefs.begin(ns.c_str(), true);
        size_t len = prefs.isKey(key.c_str()) ? prefs.getBytesLength(key.c_str()) : 0;
        prefs.end();
        return len;
    }
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
};

#define PREFS_NAMESPACE "reflow"

inline AsyncPreferenceKV prefsKV;
