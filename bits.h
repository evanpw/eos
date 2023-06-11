#pragma once
#include <stdint.h>

#include "assertions.h"

template <typename T>
inline T lowBits(T value, int count) {
    ASSERT(count >= 0 && count <= sizeof(T) * 8);
    return value & ((T(1) << count) - 1);
}

template <typename T>
inline T highBits(T value, int count) {
    ASSERT(count >= 0 && count <= sizeof(T) * 8);
    return value >> (sizeof(T) * 8 - count);
}

template <typename T>
inline T bitRange(T value, int start, int length) {
    ASSERT(start >= 0 && start <= sizeof(T) * 8 && length >= 0 &&
           start + length <= sizeof(T) * 8);
    return lowBits(highBits(value, sizeof(T) * 8 - start), length);
}

template <typename T>
inline T clearLowBits(T value, int count) {
    ASSERT(count >= 0 && count <= sizeof(T) * 8);
    return value & ~((T(1) << count) - 1);
}

inline uint64_t concatBits(uint32_t high, uint32_t low) {
    return ((uint64_t)high << 32) | low;
}
