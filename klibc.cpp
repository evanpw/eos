#include <stdint.h>
#include <string.h>

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = static_cast<uint8_t*>(dest);
    const uint8_t* s = static_cast<const uint8_t*>(src);

    for (size_t i = 0; i < n; ++i) {
        *d++ = *s++;
    }

    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* dest = static_cast<uint8_t*>(s);
    for (size_t i = 0; i < n; ++i) {
        *dest++ = c;
    }

    return s;
}

size_t strlen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return p - s;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    const char* lhs = s1;
    const char* rhs = s2;

    if (n == 0) return 0;

    for (size_t i = 0; i < n; ++i) {
        if (*lhs == 0 || *rhs == 0 || *lhs != *rhs) {
            return *lhs - *rhs;
        }

        ++lhs;
        ++rhs;
    }

    return 0;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    const char* s = src;

    for (size_t i = 0; i < n; ++i) {
        *d++ = *s;
        if (*s) ++s;
    }

    return dest;
}

const char* strchr(const char* s, int c) {
    const char* p = s;
    while (*p) {
        if (*p == c) return p;
        ++p;
    }

    return nullptr;
}
