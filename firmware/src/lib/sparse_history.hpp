#pragma once

#include <vector>
#include <cmath>
#include <cstdint>

// TODO: Consider use M4 Aggregate for packing

class SparseHistory {
public:
    struct Point { uint32_t x; int32_t y; };
    std::vector<Point> data;

    SparseHistory() {
        set_params(10, 1, 400);
    }

    void set_params(uint32_t _x_threshold, int32_t _y_threshold, uint32_t _x_scale_after) {
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

    virtual void lock() {}
    virtual void unlock() {}

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

    // Thresholds for delta encoding
    uint32_t x_threshold;
    int32_t y_threshold;

    // Boundary to start increasing x_threshold (useful for long charts)
    uint32_t x_scale_after;
};