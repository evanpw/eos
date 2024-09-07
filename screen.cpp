#include "screen.h"

#include "estd/assertions.h"
#include "estd/bits.h"
#include "estd/print.h"
#include "io.h"

// VGA I/O ports
enum : uint16_t {
    VGA_INDEX = 0x3D4,
    VGA_DATA = 0x3D5,
};

// VGA registers
enum : uint8_t {
    CURSOR_ADDRESS_MSB = 0x0E,
    CURSOR_ADDRESS_LSB = 0x0F,
};

static void writeVGARegister(uint8_t reg, uint8_t value) {
    outb(VGA_INDEX, reg);
    outb(VGA_DATA, value);
}

Screen::Screen() {
    clear();
    setCursor(0, 0);

    println("Screen initialized");
}

void Screen::clear(Color bg) {
    for (size_t y = 0; y < _height; ++y) {
        for (size_t x = 0; x < _width; ++x) {
            putChar(x, y, ' ', bg, LightGrey);
        }
    }
}

void Screen::putChar(size_t x, size_t y, char c, Color bg, Color fg) {
    ASSERT(x < _width && y < _height);

    size_t offset = y * _width + x;
    uint16_t value = (bg << 12) | (fg << 8) | c;
    _vram[offset] = value;
}

void Screen::setCursor(size_t x, size_t y) {
    ASSERT(x < _width && y < _height);

    uint16_t position = y * _width + x;
    writeVGARegister(CURSOR_ADDRESS_LSB, bitSlice(position, 0, 8));
    writeVGARegister(CURSOR_ADDRESS_MSB, bitSlice(position, 8, 16));
}

void Screen::scrollUp() {
    // Copy everything up one line
    for (size_t y = 1; y < _height; ++y) {
        for (size_t x = 0; x < _width; ++x) {
            _vram[(y - 1) * _width + x] = _vram[y * _width + x];
        }
    }

    // Clear the bottom line
    for (size_t x = 0; x < _width; ++x) {
        putChar(x, _height - 1, ' ', Black, Black);
    }
}
