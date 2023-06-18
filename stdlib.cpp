#include "stdlib.h"

void* memset(void* dest, uint8_t value, size_t n) {
    uint8_t* ptr = static_cast<uint8_t*>(dest);

    for (size_t i = 0; i < n; ++i) {
        *ptr++ = value;
    }

    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* destPtr = static_cast<uint8_t*>(dest);
    const uint8_t* srcPtr = static_cast<const uint8_t*>(src);

    for (size_t i = 0; i < n; ++i) {
        *destPtr++ = *srcPtr++;
    }

    return dest;
}

size_t strlen(const char* str) {
    size_t size = 0;
    while (*str++) ++size;
    return size;
}

char* strncpy(char* dest, const char* src, size_t n) {
    const char* ptr = src;

    for (size_t i = 0; i < n; ++i) {
        dest[i] = *ptr;
        if (*ptr) ++ptr;
    }

    return dest;
}

int strncmp(const char* str1, const char* str2, size_t num) {
    for (size_t i = 0; i < num; ++i) {
        if (str1[i] < str2[i]) {
            return -1;
        } else if (str1[i] > str2[i]) {
            return 1;
        } else if (str1[i] == 0 && str2[i] == 0) {
            return 0;
        }
    }

    return 0;
}
