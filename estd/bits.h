#pragma once
#include <stdint.h>

template <typename T>
inline T lowMask(uint8_t count) {
    return (T(1) << count) - 1;
}

template <typename T>
inline T rangeMask(uint8_t start, uint8_t length) {
    return lowMask<T>(length) << start;
}

template <typename T>
inline T lowBits(T value, uint8_t count) {
    return value & lowMask<T>(count);
}

template <typename T>
inline T highBits(T value, uint8_t count) {
    uint8_t start = sizeof(T) * 8 - count;
    return value >> start;
}

template <typename T>
inline T bitRange(T value, uint8_t start, uint8_t length) {
    return (value & rangeMask<T>(start, length)) >> start;
}

template <typename T>
inline T bitSlice(T value, uint8_t start, uint8_t end = sizeof(T) * 8) {
    int length = end - start;
    return (value >> start) & lowMask<T>(length);
}

template <typename T>
inline T clearLowBits(T value, uint8_t count) {
    return value & ~lowMask<T>(count);
}

inline uint64_t concatBits(uint32_t high, uint32_t low) {
    return ((uint64_t)high << 32) | low;
}

inline uint32_t concatBits(uint16_t high, uint16_t low) {
    return ((uint32_t)high << 16) | low;
}

inline uint16_t concatBits(uint8_t high, uint8_t low) {
    return ((uint16_t)high << 8) | low;
}

inline uint32_t concatBits(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (a << 24) | (b << 16) | (c << 8) | d;
}

template <typename T>
inline bool checkBit(T value, uint8_t bit) {
    return value & (T(1) << bit);
}

template <typename T>
inline T setBit(T value, uint8_t bit) {
    return value | (T(1) << bit);
}

template <typename T>
inline T clearBit(T value, uint8_t bit) {
    return value & ~(T(1) << bit);
}

template <typename T>
inline T clearBitRange(T value, uint8_t start, uint8_t length) {
    return value & ~rangeMask<T>(start, length);
}

template <typename T, typename U>
inline T setBitRange(T value, uint8_t start, uint8_t length, U bits) {
    return clearBitRange(value, start, length) | (T(bits) << start);
}
