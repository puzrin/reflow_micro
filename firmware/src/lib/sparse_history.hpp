#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include "freertos/semphr.h"

// TODO: Consider use M4 Aggregate for packing

class SparseHistory {
private:
    struct Point { uint32_t x; int32_t y; };

public:
    SparseHistory() {
        mutex = xSemaphoreCreateMutex();

        constexpr uint32_t x_mult = 1000;
        constexpr uint32_t y_mult = 16;

        set_params(x_mult, y_mult, 2 * x_mult, 1 * y_mult, 400 * x_mult);
    }

    ~SparseHistory() {
        vSemaphoreDelete(mutex);
    }

    void set_params(uint32_t _x_multiplier, int32_t _y_multiplier,
                   uint32_t _x_threshold, int32_t _y_threshold,
                   uint32_t _x_scale_after) {
        x_multiplier = _x_multiplier;
        y_multiplier = _y_multiplier;
        x_threshold = _x_threshold;
        y_threshold = _y_threshold;
        x_scale_after = _x_scale_after;
    }

    void reset() { lock(); data.clear(); unlock();}

    void add(uint32_t x, int32_t y) {
        lock();

        if (data.size() && data.back().x == x && data.back().y == y) {
            unlock();
            return;
        }

        Point point{x, y};
        if (is_last_point_landed()) data.push_back(point);
        else data.back() = point;

        unlock();
    }

    void add(uint32_t x, float y) {
        add(x, static_cast<int32_t>(std::round(y * y_multiplier)));
    }

private:
    bool is_last_point_landed() {
        if (data.size() < 2) return true;

        const auto& last = data.back();
        const auto& prev = data[data.size() - 2];

        if (std::abs(last.y - prev.y) >= y_threshold) return true;

        auto threshold = std::max(x_threshold, last.x / x_scale_after);
        if (last.x - prev.x >= threshold) return true;

        return false;
    }

    void lock() { xSemaphoreTake(mutex, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(mutex); }

    std::vector<Point> data;
    SemaphoreHandle_t mutex;

    // Conversion factors from external to internal representation
    uint32_t x_multiplier;
    int32_t y_multiplier;

    // Thresholds for delta encoding
    uint32_t x_threshold;
    int32_t y_threshold;

    // Boundary to start increasing x_threshold (useful for long charts)
    uint32_t x_scale_after;
};
