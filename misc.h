#pragma once
#include <stddef.h>

void halt();
void panic();

template <typename T>
void swap(T& lhs, T& rhs) {
    T tmp = lhs;
    lhs = rhs;
    rhs = tmp;
}

#define ASSERT(expr) \
    do {             \
        if (!(expr)) { \
            panic(); \
        }            \
    } while (0)
