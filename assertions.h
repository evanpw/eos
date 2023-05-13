#pragma once
#include <stddef.h>

void halt();
void panic();

#define ASSERT(expr) \
    do {             \
        if (!(expr)) { \
            panic(); \
        }            \
    } while (0)
