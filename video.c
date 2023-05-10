#include "video.h"

void clearScreen(uint8_t color) {
    char* screen = (char*)0xB8000;

    for (int i = 0; i < 80 * 25; ++i) {
        screen[2 * i] = 0x20;
        screen[2 * i + 1] = color;
    }
}

void kputc(char c) {
    asm volatile("outb %0, $0xE9" ::"a"(c));
}

// Output the number of spaces (or zeros if useZeros) to pad from
// n to padding
void pad(int n, int padding, int useZeros) {
    if (n >= padding)
        return;

    char padChar = useZeros ? '0' : ' ';
    for (int i = 0; i < (padding - n); ++i) {
        kputc(padChar);
    }
}

char* ustr(uint64_t value, char* buffer, int radix) {
    // We only handle binary to hex
    if (radix < 2 || radix > 16)
        return 0;

    char digits[] = "0123456789ABCDEF";
    char* p = buffer;

    // Get the digits (will be in reverse order)
    do {
        *(p++) = digits[value % radix];
        value /= radix;
    } while (value != 0);

    // Reverse the string
    int count = p - buffer;
    for (int i = 0; i < count / 2; ++i) {
        char c = buffer[i];
        buffer[i] = buffer[count - i - 1];
        buffer[count - i - 1] = c;
    }

    *p = 0;
    return buffer;
}

void puts(const char* msg) {
    for (const char* p = msg; *p != '\0'; ++p) {
        kputc(*p);
    }
}

size_t strlen(const char* s) {
    const char* p = s;

    while (*p != '\0') {
        ++p;
    }

    return (size_t)(p - s);
}
