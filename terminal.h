// TTY driver combining the keyboard and the screen
#pragma once
#include "estd/ring_buffer.h"
#include "estd/small_vector.h"
#include "file.h"
#include "keyboard.h"
#include "scheduler.h"
#include "screen.h"
#include "spinlock.h"

static constexpr size_t TERMINAL_INPUT_BUFFER_SIZE = 1024;
static constexpr size_t TERMINAL_OUTPUT_BUFFER_SIZE = 64;

class Terminal : public File, public KeyboardListener {
public:
    // From KeyboardListener
    void onKeyEvent(const KeyboardEvent& event) override;

    // From File
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;

private:
    friend class System;
    Terminal(KeyboardDevice& keyboard, Screen& screen);

    bool handleInput(char c);
    void handleOutput(char c);
    void handleEscapeSequence();
    bool parseEscapeSequence();
    void echo(char c);
    void newline();
    void backspace();

    KeyboardDevice& _keyboard;
    Screen& _screen;

    Spinlock _lock;

    // Input / keyboard
    RingBuffer<char, TERMINAL_INPUT_BUFFER_SIZE> _inputBuffer;
    size_t _inputLines = 0;
    estd::shared_ptr<Blocker> _inputBlocker;

    // Output / screen
    size_t _x = 0, _y = 0;
    size_t _savedX = 0, _savedY = 0;
    Screen::Color _fg = Screen::LightGrey, _bg = Screen::Black;
    SmallVector<char, TERMINAL_OUTPUT_BUFFER_SIZE> _outputBuffer;
};
