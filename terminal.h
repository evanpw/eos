#pragma once
#include "file.h"
#include "keyboard.h"
#include "ring_buffer.h"
#include "screen.h"
#include "spinlock.h"

static constexpr size_t TERMINAL_INPUT_BUFFER_SIZE = 1024;

class Terminal : public File, public KeyboardListener {
public:
    // From KeyboardListener
    void onKeyEvent(const KeyboardEvent& event) override;

    // From File
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer,
                  size_t count) override;

private:
    friend class System;
    Terminal(KeyboardDevice& keyboard, Screen& screen);

    void echo(char c);
    void newline();
    void backspace();

    KeyboardDevice& _keyboard;
    Screen& _screen;

    RingBuffer<char, TERMINAL_INPUT_BUFFER_SIZE> _inputBuffer;

    // To protect _inputBuffer
    Spinlock _lock;

    // Cursor
    size_t _x = 0;
    size_t _y = 0;
};
