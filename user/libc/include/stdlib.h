#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

[[noreturn]] void exit(int status);

#ifdef __cplusplus
}
#endif
