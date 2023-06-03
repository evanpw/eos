#include "keyboard.h"

#include <stdint.h>

#include "io.h"
#include "print.h"
#include "system.h"

// PS/2 controller I/O ports
enum : uint16_t {
    PS2_DATA = 0x60,
    PS2_STATUS = 0x64,
    PS2_COMMAND = 0x64,
};

// PS/2 Keyboard IRQ
void __attribute__((interrupt)) irqHandler1(InterruptFrame* frame) {
    if (inb(PS2_STATUS) & 1) {
        uint8_t byte = inb(PS2_DATA);
        System::keyboard().handleKey(byte);
    }

    // EOI signal
    outb(PIC1_COMMAND, EOI);
}

KeyboardDevice::KeyboardDevice() {
    // Flush the output buffer
    while (inb(PS2_STATUS) & 1) {
        inb(PS2_DATA);
        iowait(100);
    }

    for (size_t i = 0; i < (size_t)KeyCode::Max; ++i) {
        _keyState[i] = false;
    }

    println("Keyboard initialized");
}

void KeyboardDevice::handleKey(uint8_t scanCode) {
    if (scanCode == 0xE0) {
        _lastE0 = true;
        return;
    }

    bool pressed = true;
    if (scanCode & 0x80) {
        pressed = false;
        scanCode = scanCode & (~0x80);
    }

    KeyCode key;
    if (!_lastE0) {
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

    _lastE0 = false;
    _keyState[(uint8_t)key] = pressed;
    println("Keyboard event: scancode={:X}, keycode={:X}, key={}, pressed={}",
            scanCode, (uint8_t)key, keyCodeToString(key), pressed);

    // TODO: dispatch the event
    KeyboardEvent event{key, pressed};
}

void KeyboardDevice::sendCommand(uint8_t value) {
    while (inb(PS2_STATUS) & 2) {
        iowait(1000);
    }

    outb(PS2_COMMAND, value);
}

void KeyboardDevice::writeData(uint8_t value) {
    while (inb(PS2_STATUS) & 2) {
        iowait(1000);
    }

    outb(PS2_DATA, value);
}

uint8_t KeyboardDevice::readData() {
    while (!(inb(PS2_STATUS) & 1)) {
        iowait(1000);
    }

    return inb(PS2_DATA);
}

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
