#pragma once

#include <vector>
#include <cmath>
#include <cstdint>

// TODO: Consider use M4 Aggregate for packing

class SparseHistory {
public:
    struct Point { int32_t x; int32_t y; };
    std::vector<Point> data{};

    void set_params(int32_t _x_threshold, int32_t _y_threshold, int32_t _x_scale_after) {
        x_threshold = _x_threshold;
        y_threshold = _y_threshold;
        x_scale_after = _x_scale_after;
    }

    void reset() { lock(); data.clear(); unlock();}

    void add(int32_t x, int32_t y) {
        lock();

        if (!data.empty() && data.back().x == x && data.back().y == y) {
            unlock();
            return;
        }

        const Point point{x, y};
        if (is_last_point_landed()) { data.push_back(point); }
        else { data.back() = point; }

        unlock();
    }

    virtual void lock() {}
    virtual void unlock() {}

private:
    bool is_last_point_landed() {
        if (data.size() < 2) { return true; }

        const auto& last = data.back();
        const auto& prev = data[data.size() - 2];

        if (std::abs(last.y - prev.y) >= y_threshold) { return true; }

        auto threshold = std::max(x_threshold, last.x / x_scale_after);
        if (last.x - prev.x >= threshold) { return true; }

        return false;
    }

    // Thresholds for delta encoding
    int32_t x_threshold{10};
    int32_t y_threshold{1};

    // Boundary to start increasing x_threshold (useful for long charts)
    int32_t x_scale_after{400};
};
