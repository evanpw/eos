// Kernel panics and CPU halts
#pragma once
#include <stddef.h>

[[noreturn]] void halt();
[[noreturn]] void panic();
[[noreturn]] void panic(const char* msg);
