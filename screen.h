#pragma once
#include <stddef.h>
#include <stdint.h>

class Screen {
public:
    enum Color : uint8_t {
        Black = 0,
        Blue = 1,
        Green = 2,
        Cyan = 3,
        Red = 4,
        Magenta = 5,
        Brown = 6,
        LightGrey = 7,
        DarkGrey = 8,
        LightBlue = 9,
        LightGreen = 10,
        LightCyan = 11,
        LightRed = 12,
        LightMagenta = 13,
        Yellow = 14,
        White = 15,
    };

public:
    size_t width() const { return _width; }
    size_t height() const { return _height; }

    void clear(Color bg = Black);
    void putChar(size_t x, size_t y, char c, Color bg, Color fg);
    void setCursor(size_t x, size_t y);

private:
    friend class System;
    Screen();

    size_t _width = 80;
    size_t _height = 25;
    uint16_t* _vram = (uint16_t*)0xB8000;
};
