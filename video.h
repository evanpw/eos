#pragma once
#include <stdint.h>
#include <stddef.h>

void clearScreen(uint8_t);
void puts(const char* msg);
char* ustr(uint64_t value, char* buffer, int radix);
void pad(int n, int padding, bool useZeros);
size_t strlen(const char* s);
