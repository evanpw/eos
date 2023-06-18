#pragma once
#include <stddef.h>

[[noreturn]] void halt();
[[noreturn]] void panic();
[[noreturn]] void panic(const char* msg);
