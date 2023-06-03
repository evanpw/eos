#pragma once
#include "keyboard.h"
#include "screen.h"

class Terminal : public KeyboardListener {
public:
    void onKeyEvent(const KeyboardEvent& event) override;

private:
    friend class System;
    Terminal(KeyboardDevice& keyboard, Screen& screen);

    void echo(char c);

    KeyboardDevice& _keyboard;
    Screen& _screen;

    // Cursor
    size_t _x = 0;
    size_t _y = 0;
};
