#include "keyboard.h"

#include <stdint.h>

#include "estd/print.h"
#include "interrupts.h"
#include "io.h"
#include "system.h"

// PS/2 controller I/O ports
enum : uint16_t {
    PS2_DATA = 0x60,
    PS2_STATUS = 0x64,
    PS2_COMMAND = 0x64,
};

// PS/2 Keyboard IRQ
void irqHandler1(TrapRegisters&) {
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

    registerIrqHandler(1, irqHandler1);

    println("kbd: init complete");
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

    if (_listener) {
        KeyboardEvent event{key, pressed};
        _listener->onKeyEvent(event);
    }
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
