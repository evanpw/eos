#pragma once
#include <stddef.h>
#include <stdint.h>

void* memset(void* dest, uint8_t value, size_t n);
size_t strlen(const char* str);

template <typename T>
T min(const T& lhs, const T& rhs) {
    return (rhs < lhs) ? rhs : lhs;
}

template <typename T>
T max(const T& lhs, const T& rhs) {
    return (lhs < rhs) ? rhs : lhs;
}
