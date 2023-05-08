#include "video.h"

void clearScreen(uint8_t color) {
    char* screen = (char*)0xB8000;

    for (int i = 0; i < 80 * 25; ++i) {
        screen[2 * i] = 0x20;
        screen[2 * i + 1] = color;
    }
}

void _dbgPrintChar(char c) {
    asm volatile("outb %0, $0xE9" ::"a"(c));
}

void puts(const char* msg) {
    for (const char* p = msg; *p != '\0'; ++p) {
        _dbgPrintChar(*p);
    }
}
