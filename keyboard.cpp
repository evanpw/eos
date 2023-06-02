#include "keyboard.h"

#include <stdint.h>

#include "ring_buffer.h"
#include "io.h"
#include "print.h"

enum : uint8_t {
    PS2_DATA = 0x60,
    PS2_STATUS = 0x64,
    PS2_COMMAND = 0x64,
};

void sendCommand(uint8_t value) {
    while (inb(PS2_STATUS) & 2) {
        iowait(1000);
    }

    outb(PS2_COMMAND, value);
}

void writeData(uint8_t value) {
    while (inb(PS2_STATUS) & 2) {
        iowait(1000);
    }

    outb(PS2_DATA, value);
}

uint8_t readData() {
    while (!(inb(PS2_STATUS) & 1)) {
        println("output not ready");
        iowait(1000);
    }

    return inb(PS2_DATA);
}

void initializeKeyboard() {
    // Flush the output buffer
    while (inb(PS2_STATUS) & 1) {
        inb(PS2_DATA);
        iowait(100);
    }
}

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
};

const char* keyCodeToString(KeyCode keyCode) {
    switch ((uint8_t)keyCode) {
        case 0x00:
            return "Unknown";
        case 0x01:
            return "Escape";
        case 0x02:
            return "One";
        case 0x03:
            return "Two";
        case 0x04:
            return "Three";
        case 0x05:
            return "Four";
        case 0x06:
            return "Five";
        case 0x07:
            return "Six";
        case 0x08:
            return "Seven";
        case 0x09:
            return "Eight";
        case 0x0A:
            return "Nine";
        case 0x0B:
            return "Zero";
        case 0x0C:
            return "Minus";
        case 0x0D:
            return "Equals";
        case 0x0E:
            return "Backspace";
        case 0x0F:
            return "Tab";
        case 0x10:
            return "Q";
        case 0x11:
            return "W";
        case 0x12:
            return "E";
        case 0x13:
            return "R";
        case 0x14:
            return "T";
        case 0x15:
            return "Y";
        case 0x16:
            return "U";
        case 0x17:
            return "I";
        case 0x18:
            return "O";
        case 0x19:
            return "P";
        case 0x1A:
            return "LBracket";
        case 0x1B:
            return "RBracket";
        case 0x1C:
            return "Enter";
        case 0x1D:
            return "LCtrl";
        case 0x1E:
            return "A";
        case 0x1F:
            return "S";
        case 0x20:
            return "D";
        case 0x21:
            return "F";
        case 0x22:
            return "G";
        case 0x23:
            return "H";
        case 0x24:
            return "J";
        case 0x25:
            return "K";
        case 0x26:
            return "L";
        case 0x27:
            return "Semicolon";
        case 0x28:
            return "Apostrophe";
        case 0x29:
            return "Backtick";
        case 0x2A:
            return "LShift";
        case 0x2B:
            return "Backslash";
        case 0x2C:
            return "Z";
        case 0x2D:
            return "X";
        case 0x2E:
            return "C";
        case 0x2F:
            return "V";
        case 0x30:
            return "B";
        case 0x31:
            return "N";
        case 0x32:
            return "M";
        case 0x33:
            return "Comma";
        case 0x34:
            return "Period";
        case 0x35:
            return "Slash";
        case 0x36:
            return "RShift";
        case 0x37:
            return "KeypadAsterisk";
        case 0x38:
            return "LAlt";
        case 0x39:
            return "Space";
        case 0x3A:
            return "CapsLock";
        case 0x3B:
            return "F1";
        case 0x3C:
            return "F2";
        case 0x3D:
            return "F3";
        case 0x3E:
            return "F4";
        case 0x3F:
            return "F5";
        case 0x40:
            return "F6";
        case 0x41:
            return "F7";
        case 0x42:
            return "F8";
        case 0x43:
            return "F9";
        case 0x44:
            return "F10";
        case 0x45:
            return "NumLock";
        case 0x46:
            return "ScrollLock";
        case 0x47:
            return "Keypad7";
        case 0x48:
            return "Keypad8";
        case 0x49:
            return "Keypad9";
        case 0x4A:
            return "KeypadMinus";
        case 0x4B:
            return "Keypad4";
        case 0x4C:
            return "Keypad5";
        case 0x4D:
            return "Keypad6";
        case 0x4E:
            return "KeypadPlus";
        case 0x4F:
            return "Keypad1";
        case 0x50:
            return "Keypad2";
        case 0x51:
            return "Keypad3";
        case 0x52:
            return "Keypad0";
        case 0x53:
            return "KeypadDot";
        case 0x57:
            return "F11";
        case 0x58:
            return "F12";
        case 0x60:
            return "KeypadEnter";
        case 0x61:
            return "RCtrl";
        case 0x62:
            return "KeypadSlash";
        case 0x63:
            return "RAlt";
        case 0x64:
            return "Home";
        case 0x65:
            return "Up";
        case 0x66:
            return "PageUp";
        case 0x67:
            return "Left";
        case 0x68:
            return "Right";
        case 0x69:
            return "End";
        case 0x6A:
            return "Down";
        case 0x6B:
            return "PageDown";
        case 0x6C:
            return "Insert";
        case 0x6D:
            return "Delete";
        case 0x6E:
            return "Menu";
        default:
            return "<unknown>";
    }
}

struct KeyboardEvent {
    KeyCode key;
    bool pressed;
};

static bool g_lastE0 = false;
static RingBuffer<KeyboardEvent, 32> g_keyQueue;

void handleKey(uint8_t scanCode) {
    if (scanCode == 0xE0) {
        g_lastE0 = true;
        return;
    }

    bool pressed = true;
    if (scanCode & 0x80) {
        pressed = false;
        scanCode = scanCode & (~0x80);
    }

    KeyCode key;
    if (!g_lastE0) {
        if (scanCode <= 0x53 || scanCode == 0x57 || scanCode == 0x58) {
            key = (KeyCode)scanCode;
        } else {
            key = KeyCode::Unknown;
        }
    } else {
        switch (scanCode) {
            case 0x1C:
                key = KeyCode::KeypadEnter;
                break;

            case 0x1D:
                key = KeyCode::RCtrl;
                break;

            case 0x35:
                key = KeyCode::KeypadSlash;
                break;

            case 0x38:
                key = KeyCode::RAlt;
                break;

            case 0x47:
                key = KeyCode::Home;
                break;

            case 0x48:
                key = KeyCode::Up;
                break;

            case 0x49:
                key = KeyCode::PageUp;
                break;

            case 0x4B:
                key = KeyCode::Left;
                break;

            case 0x4D:
                key = KeyCode::Right;
                break;

            case 0x4F:
                key = KeyCode::End;
                break;

            case 0x50:
                key = KeyCode::Down;
                break;

            case 0x51:
                key = KeyCode::PageDown;
                break;

            case 0x52:
                key = KeyCode::Insert;
                break;

            case 0x53:
                key = KeyCode::Delete;
                break;

            case 0x5D:
                key = KeyCode::Menu;
                break;

            default:
                key = KeyCode::Unknown;
                break;
        }
    }

    KeyboardEvent event{key, pressed};
    g_keyQueue.push(event);
    g_lastE0 = false;
    println("Keyboard event: scancode={:X}, key={}, pressed={}", scanCode, keyCodeToString(event.key), pressed);
}

// PS/2 Keyboard IRQ
void __attribute__((interrupt)) irqHandler1(InterruptFrame* frame) {
    if (inb(PS2_STATUS) & 1) {
        uint8_t byte = inb(PS2_DATA);
        handleKey(byte);
    }

    // EOI signal
    outb(PIC1_COMMAND, EOI);
}
