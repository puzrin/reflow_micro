#pragma once

#include <stdint.h>
static inline uint32_t isqrt32(uint32_t x) {
    if (!x) return 0;
#if defined(__GNUC__) || defined(__clang__)
    unsigned shift = (31u - __builtin_clz(x)) & ~1u;
    uint32_t bit = 1u << shift;
#else
    uint32_t bit = 1u << 30; // 2^(2*k), k=max
    while (bit > x) bit >>= 2;
#endif
    uint32_t r = 0;
    while (bit) {
        if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; }
        else { r >>= 1; }
        bit >>= 2;
    }
    return r;
}

static inline uint64_t isqrt64(uint64_t x) {
    if (!x) return 0;
#if defined(__GNUC__) || defined(__clang__)
    unsigned shift = (63u - __builtin_clzll(x)) & ~1u;
    uint64_t bit = 1ull << shift;
#else
    uint64_t bit = 1ull << 62;
    while (bit > x) bit >>= 2;
#endif
    uint64_t r = 0;
    while (bit) {
        if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; }
        else { r >>= 1; }
        bit >>= 2;
    }
    return r;
}
