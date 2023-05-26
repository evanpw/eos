#pragma once
#include <stdint.h>

inline uint64_t lowBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value & ((1 << count) - 1);
}

inline uint64_t highBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value >> (64 - count);
}

inline uint64_t bitRange(uint64_t value, int start, int length) {
    ASSERT(start >= 0 && start <= 64 && length >= 0 && start + length <= 64);
    return lowBits(highBits(value, 64 - start), length);
}

inline uint64_t clearLowBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value & ~((1 << count) - 1);
}
