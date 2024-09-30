#include "estd/print.h"

#include <string.h>

#ifdef KERNEL
#include "io.h"
void printChar(char c) { outb(0xE9, c); }
#else
#include <stdio.h>
void printChar(char c) { putchar(c); }
#endif

void printInt(const FormatSpec& spec, uint64_t value) {
    ASSERT(spec.base >= 2 && spec.base <= 16);

    constexpr const char* uppercaseDigits = "0123456789ABCDEF";
    constexpr const char* lowercaseDigits = "0123456789abcdef";
    const char* lookup = spec.uppercase ? uppercaseDigits : lowercaseDigits;

    // Get the digits (will be in reverse order)
    char digits[64];
    char* p = digits;
    do {
        *(p++) = lookup[value % spec.base];
        value /= spec.base;
    } while (value != 0);

    size_t digitLength = p - digits;

    // Add padding if necessary
    for (size_t i = digitLength; i < spec.padTo; ++i) {
        printChar(spec.padChar);
    }

    // Append the digits in reversed (correct) order
    for (size_t i = 0; i < digitLength; ++i) {
        printChar(digits[digitLength - 1 - i]);
    }
}

void printString(const FormatSpec& spec, const char* str) {
    size_t length = strlen(str);

    // Add padding if necessary
    for (size_t i = length; i < spec.padTo; ++i) {
        printChar(spec.padChar);
    }

    // Print the string char-by-char
    while (*str) {
        printChar(*str++);
    }
}

bool FormatStringParser::accept(char c) {
    if (peek() == c) {
        next();
        return true;
    }

    return false;
}

void FormatStringParser::expect(char c) {
    ASSERT(peek() == c);
    next();
}

size_t FormatStringParser::parseInteger() {
    size_t value = 0;

    while (peek() >= '0' && peek() <= '9') {
        int digit = next() - '0';
        value = value * 10 + digit;
    }

    return value;
}

FormatSpec FormatStringParser::parseFormatSpec() {
    FormatSpec spec;

    // Format specification
    if (accept(':')) {
        // Padding character
        if (accept('0')) {
            spec.padChar = '0';
        }

        // Width
        if (peek() >= '0' && peek() <= '9') {
            spec.padTo = parseInteger();
        }

        // Type
        if (accept('b')) {
            spec.base = 2;
        } else if (accept('d')) {
            spec.base = 10;
        } else if (accept('x')) {
            spec.base = 16;
        } else if (accept('X')) {
            spec.base = 16;
            spec.uppercase = true;
        } else {
            ASSERT(false);
        }
    }

    // End of replacement field
    expect('}');

    return spec;
}

void _printImpl(FormatStringParser& parser, FormatArgs& args) {
    while (parser) {
        if (parser.accept('{')) {
            FormatSpec spec = parser.parseFormatSpec();
            const FormatArgBase& arg = args.next();
            arg.print(spec);
        } else {
            printChar(parser.next());
        }
    }
}
