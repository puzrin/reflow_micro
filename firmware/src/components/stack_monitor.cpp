#include "stack_monitor.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "logger.hpp"

void stack_monitor_start() {
    xTaskCreate([](void* pvParameters) {
        (void)pvParameters;

        const UBaseType_t MAX_TASKS = 20;
        TaskStatus_t taskStatusArray[MAX_TASKS];

        while (true) {
            UBaseType_t actualNumber = uxTaskGetSystemState(taskStatusArray, MAX_TASKS, NULL);

            if(actualNumber > MAX_TASKS) {
                DEBUG("Error: Too many tasks to monitor. Tasks: {}, Max: {}",
                    uint32_t(actualNumber), uint32_t(MAX_TASKS));
            } else {
                DEBUG("################## Stack Monitor ##################");
                for(UBaseType_t i = 0; i < actualNumber; i++) {
                    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(taskStatusArray[i].xHandle);
                    DEBUG("Task: {} Stack HWM: {} words",
                        taskStatusArray[i].pcTaskName,
                        uint32_t(stackHighWaterMark));
                }
                DEBUG("---------------------------------------------------");
            }

            vTaskDelay(pdMS_TO_TICKS(60 * 1000));
        }
    }, "StackMonitorTask", 1024 * 4, NULL, 0, NULL);
}