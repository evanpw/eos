#pragma once
#include <stdint.h>
#include <stddef.h>

void clearScreen(uint8_t);
void puts(const char* msg);
size_t ustr(uint64_t value, char* buffer, int radix, int padTo = 0, char padChar = ' ');
