#pragma once
#include <stddef.h>

[[noreturn]] void __assertion_failed(const char* msg);

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define ASSERT(expr)                             \
    do {                                         \
        if (!(expr)) {                           \
            __assertion_failed("assertion '" #expr "' failed at line " STRINGIZE( \
                __LINE__) " in file " __FILE__); \
        }                                        \
    } while (0)
