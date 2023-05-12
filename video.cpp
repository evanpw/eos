#include "video.h"
#include "cstring.h"
#include "misc.h"

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

size_t ustr(uint64_t value, char* buffer, int radix, int padTo, char padChar) {
    ASSERT(radix >= 2 && radix <= 16);

    char digits[] = "0123456789ABCDEF";
    char* p = buffer;

    // Get the digits (will be in reverse order)
    do {
        *(p++) = digits[value % radix];
        value /= radix;
    } while (value != 0);

    // Reverse the string
    size_t length = p - buffer;
    for (size_t i = 0; i < length / 2; ++i) {
        swap(buffer[length - i - 1], buffer[i]);
    }

    *p = 0;

    // Pad if necessary
    if (length < padTo) {
        // Move the unpadded digits to their final location
        int neededPadding = padTo - length;
        memmove(buffer + neededPadding, buffer, length + 1);

        // Fill in the beginning of the string with zeros or spaces
        for (int i = 0; i < neededPadding; ++i) {
            buffer[i] = padChar;
        }

        length = padTo;
    }

    return length;
}

void puts(const char* msg) {
    for (const char* p = msg; *p != '\0'; ++p) {
        kputc(*p);
    }
}
