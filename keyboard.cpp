#include "keyboard.h"

#include <stdint.h>

#include "io.h"
#include "print.h"
#include "ring_buffer.h"

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

    println("Keyboard initialized");
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
    switch (keyCode) {
        case KeyCode::Unknown:
            return "Unknown";
        case KeyCode::Escape:
            return "Escape";
        case KeyCode::One:
            return "One";
        case KeyCode::Two:
            return "Two";
        case KeyCode::Three:
            return "Three";
        case KeyCode::Four:
            return "Four";
        case KeyCode::Five:
            return "Five";
        case KeyCode::Six:
            return "Six";
        case KeyCode::Seven:
            return "Seven";
        case KeyCode::Eight:
            return "Eight";
        case KeyCode::Nine:
            return "Nine";
        case KeyCode::Zero:
            return "Zero";
        case KeyCode::Minus:
            return "Minus";
        case KeyCode::Equals:
            return "Equals";
        case KeyCode::Backspace:
            return "Backspace";
        case KeyCode::Tab:
            return "Tab";
        case KeyCode::Q:
            return "Q";
        case KeyCode::W:
            return "W";
        case KeyCode::E:
            return "E";
        case KeyCode::R:
            return "R";
        case KeyCode::T:
            return "T";
        case KeyCode::Y:
            return "Y";
        case KeyCode::U:
            return "U";
        case KeyCode::I:
            return "I";
        case KeyCode::O:
            return "O";
        case KeyCode::P:
            return "P";
        case KeyCode::LBracket:
            return "LBracket";
        case KeyCode::RBracket:
            return "RBracket";
        case KeyCode::Enter:
            return "Enter";
        case KeyCode::LCtrl:
            return "LCtrl";
        case KeyCode::A:
            return "A";
        case KeyCode::S:
            return "S";
        case KeyCode::D:
            return "D";
        case KeyCode::F:
            return "F";
        case KeyCode::G:
            return "G";
        case KeyCode::H:
            return "H";
        case KeyCode::J:
            return "J";
        case KeyCode::K:
            return "K";
        case KeyCode::L:
            return "L";
        case KeyCode::Semicolon:
            return "Semicolon";
        case KeyCode::Apostrophe:
            return "Apostrophe";
        case KeyCode::Backtick:
            return "Backtick";
        case KeyCode::LShift:
            return "LShift";
        case KeyCode::Backslash:
            return "Backslash";
        case KeyCode::Z:
            return "Z";
        case KeyCode::X:
            return "X";
        case KeyCode::C:
            return "C";
        case KeyCode::V:
            return "V";
        case KeyCode::B:
            return "B";
        case KeyCode::N:
            return "N";
        case KeyCode::M:
            return "M";
        case KeyCode::Comma:
            return "Comma";
        case KeyCode::Period:
            return "Period";
        case KeyCode::Slash:
            return "Slash";
        case KeyCode::RShift:
            return "RShift";
        case KeyCode::KeypadAsterisk:
            return "KeypadAsterisk";
        case KeyCode::LAlt:
            return "LAlt";
        case KeyCode::Space:
            return "Space";
        case KeyCode::CapsLock:
            return "CapsLock";
        case KeyCode::F1:
            return "F1";
        case KeyCode::F2:
            return "F2";
        case KeyCode::F3:
            return "F3";
        case KeyCode::F4:
            return "F4";
        case KeyCode::F5:
            return "F5";
        case KeyCode::F6:
            return "F6";
        case KeyCode::F7:
            return "F7";
        case KeyCode::F8:
            return "F8";
        case KeyCode::F9:
            return "F9";
        case KeyCode::F10:
            return "F10";
        case KeyCode::NumLock:
            return "NumLock";
        case KeyCode::ScrollLock:
            return "ScrollLock";
        case KeyCode::Keypad7:
            return "Keypad7";
        case KeyCode::Keypad8:
            return "Keypad8";
        case KeyCode::Keypad9:
            return "Keypad9";
        case KeyCode::KeypadMinus:
            return "KeypadMinus";
        case KeyCode::Keypad4:
            return "Keypad4";
        case KeyCode::Keypad5:
            return "Keypad5";
        case KeyCode::Keypad6:
            return "Keypad6";
        case KeyCode::KeypadPlus:
            return "KeypadPlus";
        case KeyCode::Keypad1:
            return "Keypad1";
        case KeyCode::Keypad2:
            return "Keypad2";
        case KeyCode::Keypad3:
            return "Keypad3";
        case KeyCode::Keypad0:
            return "Keypad0";
        case KeyCode::KeypadDot:
            return "KeypadDot";
        case KeyCode::F11:
            return "F11";
        case KeyCode::F12:
            return "F12";
        case KeyCode::KeypadEnter:
            return "KeypadEnter";
        case KeyCode::RCtrl:
            return "RCtrl";
        case KeyCode::KeypadSlash:
            return "KeypadSlash";
        case KeyCode::RAlt:
            return "RAlt";
        case KeyCode::Home:
            return "Home";
        case KeyCode::Up:
            return "Up";
        case KeyCode::PageUp:
            return "PageUp";
        case KeyCode::Left:
            return "Left";
        case KeyCode::Right:
            return "Right";
        case KeyCode::End:
            return "End";
        case KeyCode::Down:
            return "Down";
        case KeyCode::PageDown:
            return "PageDown";
        case KeyCode::Insert:
            return "Insert";
        case KeyCode::Delete:
            return "Delete";
        case KeyCode::Menu:
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
    println("Keyboard event: scancode={:X}, key={}, pressed={}", scanCode,
            keyCodeToString(event.key), pressed);
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
