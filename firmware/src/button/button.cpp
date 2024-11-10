#include "button.hpp"
#include "logger.hpp"
#include "app.hpp"

Button button;

void button_thread(void* pvParameters) {
    while (true) {
        button.tick(millis());
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void button_init() {
    button.setEventHandler([](ButtonEventId event) {
        app.receive(ButtonAction(event));
    });

    xTaskCreate(button_thread, "button_thread", 1024 * 4, NULL, 1, NULL);
}
