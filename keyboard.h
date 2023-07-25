// Keyboard drive
#pragma once
#include "interrupts.h"
#include "trap.h"

void irqHandler1(TrapRegisters& regs);

enum class KeyCode : uint8_t {
    // Identity-mapped keycodes
    Unknown = 0x00,
    Escape = 0x01,
    One = 0x02,
    Two = 0x03,
    Three = 0x04,
    Four = 0x05,
    Five = 0x06,
    Six = 0x07,
    Seven = 0x08,
    Eight = 0x09,
    Nine = 0x0A,
    Zero = 0x0B,
    Minus = 0x0C,
    Equals = 0x0D,
    Backspace = 0x0E,
    Tab = 0x0F,
    Q = 0x10,
    W = 0x11,
    E = 0x12,
    R = 0x13,
    T = 0x14,
    Y = 0x15,
    U = 0x16,
    I = 0x17,
    O = 0x18,
    P = 0x19,
    LBracket = 0x1A,
    RBracket = 0x1B,
    Enter = 0x1C,
    LCtrl = 0x1D,
    A = 0x1E,
    S = 0x1F,
    D = 0x20,
    F = 0x21,
    G = 0x22,
    H = 0x23,
    J = 0x24,
    K = 0x25,
    L = 0x26,
    Semicolon = 0x27,
    Apostrophe = 0x28,
    Backtick = 0x29,
    LShift = 0x2A,
    Backslash = 0x2B,
    Z = 0x2C,
    X = 0x2D,
    C = 0x2E,
    V = 0x2F,
    B = 0x30,
    N = 0x31,
    M = 0x32,
    Comma = 0x33,
    Period = 0x34,
    Slash = 0x35,
    RShift = 0x36,
    KeypadAsterisk = 0x37,
    LAlt = 0x38,
    Space = 0x39,
    CapsLock = 0x3A,
    F1 = 0x3B,
    F2 = 0x3C,
    F3 = 0x3D,
    F4 = 0x3E,
    F5 = 0x3F,
    F6 = 0x40,
    F7 = 0x41,
    F8 = 0x42,
    F9 = 0x43,
    F10 = 0x44,
    NumLock = 0x45,
    ScrollLock = 0x46,
    Keypad7 = 0x47,
    Keypad8 = 0x48,
    Keypad9 = 0x49,
    KeypadMinus = 0x4A,
    Keypad4 = 0x4B,
    Keypad5 = 0x4C,
    Keypad6 = 0x4D,
    KeypadPlus = 0x4E,
    Keypad1 = 0x4F,
    Keypad2 = 0x50,
    Keypad3 = 0x51,
    Keypad0 = 0x52,
    KeypadDot = 0x53,
    F11 = 0x57,
    F12 = 0x58,

    // Non-identity-mapped keycodes
    KeypadEnter = 0x60,
    RCtrl = 0x61,
    KeypadSlash = 0x62,
    RAlt = 0x63,
    Home = 0x64,
    Up = 0x65,
    PageUp = 0x66,
    Left = 0x67,
    Right = 0x68,
    End = 0x69,
    Down = 0x6A,
    PageDown = 0x6B,
    Insert = 0x6C,
    Delete = 0x6D,
    Menu = 0x6E,

    Max = 0x6E,
};

struct KeyboardEvent {
    KeyCode key;
    bool pressed;
};

struct KeyboardListener {
    virtual void onKeyEvent(const KeyboardEvent& event) = 0;
};

class KeyboardDevice {
public:
    void addListener(KeyboardListener* listener) {
        // TODO: needs locking
        ASSERT(!_listener);
        _listener = listener;
    }

    bool isPressed(KeyCode keycode) {
        // TODO: needs locking
        return _keyState[(size_t)keycode];
    }

private:
    friend class System;
    KeyboardDevice();

    friend void irqHandler1(TrapRegisters& regs);
    void handleKey(uint8_t scanCode);

    bool _lastE0 = false;
    bool _keyState[(size_t)KeyCode::Max];  // TODO: use a bitmap
    KeyboardListener* _listener = nullptr;

private:
    // PS/2 device communication
    void sendCommand(uint8_t value);
    void writeData(uint8_t value);
    uint8_t readData();
};
