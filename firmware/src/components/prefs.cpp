#include <Arduino.h>
#include "prefs.hpp"

AsyncPreferenceKV prefsKV;
AsyncPreferenceWriter prefsWriter(200, []() { return millis(); });

void prefs_init() {
    // Flash writes can be blocking, use separate task with lowest priority.
    xTaskCreate([](void*) {
        while(true) {
            prefsWriter.tick();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }, "prefs", 1024*4, NULL, 0, NULL);
}
