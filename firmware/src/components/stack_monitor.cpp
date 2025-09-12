#include "stack_monitor.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "logger.hpp"

void stack_monitor_start() {
    xTaskCreate([](void* /*pvParameters*/) {
        const UBaseType_t MAX_TASKS = 20;
        TaskStatus_t taskStatusArray[MAX_TASKS];

        while (true) {
            UBaseType_t actualNumber = uxTaskGetSystemState(taskStatusArray, MAX_TASKS, NULL);

            if(actualNumber > MAX_TASKS) {
                APP_LOGI("Error: Too many tasks to monitor. Tasks: {}, Max: {}",
                    uint32_t(actualNumber), uint32_t(MAX_TASKS));
            } else {
                APP_LOGI("################## Stack Monitor ##################");
                for(UBaseType_t i = 0; i < actualNumber; i++) {
                    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(taskStatusArray[i].xHandle);
                    APP_LOGI("Task: {} Stack HWM: {} words",
                        taskStatusArray[i].pcTaskName,
                        uint32_t(stackHighWaterMark));
                }
                APP_LOGI("Free memory: {}; Minimum free memory: {}; Max free block: {}",
                    heap_caps_get_free_size(MALLOC_CAP_8BIT),
                    heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                    heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

                APP_LOGI("---------------------------------------------------");
            }

            vTaskDelay(pdMS_TO_TICKS(60 * 1000));
        }
    }, "StackMonitorTask", 1024 * 2, NULL, 0, NULL);
}