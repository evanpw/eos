#pragma once
#include <stddef.h>

[[noreturn]] void halt();
[[noreturn]] void panic();
[[noreturn]] void panic(const char* msg);

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define ASSERT(expr)                             \
    do {                                         \
        if (!(expr)) {                           \
            panic("assertion '" #expr "' failed at line " STRINGIZE( \
                __LINE__) " in file " __FILE__); \
        }                                        \
    } while (0)
