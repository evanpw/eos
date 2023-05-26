#include "stdlib.h"

void* memset(void* dest, uint8_t value, size_t n) {
    uint8_t* ptr = static_cast<uint8_t*>(dest);

    for (size_t i = 0; i < n; ++i) {
        *ptr++ = value;
    }

    return dest;
}
