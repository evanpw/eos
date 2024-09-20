#pragma once
#include <stddef.h>

extern "C" {

void* malloc(size_t size);
void free(void* ptr);

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

[[noreturn]] void exit(int status);
}
