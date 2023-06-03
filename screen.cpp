#include "screen.h"

#include "assertions.h"
#include "bits.h"
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

Screen::Screen(size_t width, size_t height) : _width(width), _height(height) {
    clear();
    setCursor(0, 0);
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
    writeVGARegister(CURSOR_ADDRESS_MSB, lowBits(position, 8));
    writeVGARegister(CURSOR_ADDRESS_LSB, highBits(position, 8));
}
