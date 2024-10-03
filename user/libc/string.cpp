#include <stdint.h>
#include <stdlib.h>
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

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = static_cast<uint8_t*>(dest);
    const uint8_t* s = static_cast<const uint8_t*>(src);

    if (d < s) {
        for (size_t i = 0; i < n; ++i) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        for (size_t i = 0; i < n; ++i) {
            *--d = *--s;
        }
    }

    return dest;
}

int memcmp(const void* p1, const void* p2, size_t n) {
    const uint8_t* lhs = static_cast<const uint8_t*>(p1);
    const uint8_t* rhs = static_cast<const uint8_t*>(p2);

    for (size_t i = 0; i < n; ++i) {
        if (*lhs != *rhs) {
            return *lhs - *rhs;
        }

        ++lhs;
        ++rhs;
    }

    return 0;
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

    for (size_t i = 0; i < n && (*lhs || *rhs); ++i) {
        if (*lhs != *rhs) {
            return (unsigned char)(*lhs) - (unsigned char)(*rhs);
        }

        ++lhs;
        ++rhs;
    }

    return 0;
}

int strcmp(const char* s1, const char* s2) {
    const char* lhs = s1;
    const char* rhs = s2;

    while (*lhs && *rhs && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }

    return (unsigned char)(*lhs) - (unsigned char)(*rhs);
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

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    const char* s = src;

    while (*s) {
        *d++ = *s++;
    }

    *d = '\0';

    return dest;
}

const char* strchr(const char* s, int c) {
    for (const char* p = s; *p; ++p) {
        if (*p == c) {
            return p;
        }
    }

    return nullptr;
}

char* strdup(const char* s) {
    size_t len = strlen(s);
    char* newStr = (char*)malloc(len + 1);
    strcpy(newStr, s);
    return newStr;
}
