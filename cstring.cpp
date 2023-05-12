#include "cstring.h"

size_t strlen(const char* str) {
    size_t length = 0;

    while (*str++) {
        ++length;
    }

    return length;
}

void* memmove(void* dest, void* src, size_t n) {
    char* cdest = (char*)dest;
    char* csrc = (char*)src;

    if (cdest - csrc >= n) {
        for (size_t i = 0; i < n; ++i) {
            cdest[i] = csrc[i];
        }
    } else {
        for (size_t i = 0; i < n; ++i) {
            cdest[n - 1 - i] = csrc[n - 1 - i];
        }
    }

    return dest;
}
