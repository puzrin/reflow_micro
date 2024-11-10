#include "blinker.h"

Blinker blinker;

void blinker_thread(void* pvParameters) {
    while (true) {
        blinker.tick(millis());
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void blinker_init() {
    xTaskCreate(blinker_thread, "blink_thread", 1024 * 4, NULL, 1, NULL);
}