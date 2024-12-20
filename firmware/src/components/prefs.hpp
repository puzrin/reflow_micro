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

extern AsyncPreferenceKV prefsKV;
extern AsyncPreferenceWriter prefsWriter;
void prefs_init();