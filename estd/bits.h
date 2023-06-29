#pragma once
#include <stdint.h>

#include "estd/assertions.h"

template <typename T>
inline T lowBits(T value, uint8_t count) {
    ASSERT(count <= sizeof(T) * 8);
    return value & ((T(1) << count) - 1);
}

template <typename T>
inline T highBits(T value, uint8_t count) {
    ASSERT(count <= sizeof(T) * 8);
    return value >> (sizeof(T) * 8 - count);
}

template <typename T>
inline T bitRange(T value, uint8_t start, uint8_t length) {
    ASSERT(start <= sizeof(T) * 8 && start + length <= sizeof(T) * 8);
    return lowBits(highBits(value, sizeof(T) * 8 - start), length);
}

template <typename T>
inline T bitSlice(T value, uint8_t start, uint8_t end = sizeof(T) * 8) {
    ASSERT(end <= sizeof(T) * 8 && start <= end);
    int length = end - start;
    return (value >> start) & ((T(1) << length) - 1);
}

template <typename T>
inline T clearLowBits(T value, uint8_t count) {
    ASSERT(count <= sizeof(T) * 8);
    return value & ~((T(1) << count) - 1);
}

inline uint64_t concatBits(uint32_t high, uint32_t low) {
    return ((uint64_t)high << 32) | low;
}

inline uint32_t concatBits(uint16_t high, uint16_t low) {
    return ((uint32_t)high << 16) | low;
}

template <typename T>
inline T checkBit(T value, uint8_t bit) {
    return value & (T(1) << bit);
}
