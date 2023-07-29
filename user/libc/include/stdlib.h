#pragma once
#include <stddef.h>

extern "C" {

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

[[noreturn]] void exit(int status);
}
