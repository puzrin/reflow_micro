#include "lib/sparse_history.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class History : public SparseHistory {
public:
    History() : mutex(xSemaphoreCreateMutex()) {}
    ~History() { vSemaphoreDelete(mutex); }

    void lock() override { xSemaphoreTake(mutex, portMAX_DELAY); }
    void unlock() override { xSemaphoreGive(mutex); }
private:
    SemaphoreHandle_t mutex;
};
