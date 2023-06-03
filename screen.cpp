#include "screen.h"

#include "assertions.h"
#include "io.h"
#include "bits.h"

Screen::Screen(size_t width, size_t height) : _width(width), _height(height)
{
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
    outb(0x3D4, 0x0F);
    outb(0x3D5, lowBits(position, 8));
    outb(0x3D4, 0x0E);
    outb(0x3D5, highBits(position, 8));
}
