#include "keyboard.h"

#include <stdint.h>

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

// PS/2 Keyboard IRQ
void __attribute__((interrupt)) irqHandler1(InterruptFrame* frame) {
    if (inb(PS2_STATUS) & 1) {
        uint8_t byte = inb(PS2_DATA);
        println("Keyboard interrupt: {:X}", byte);
    } else {
        println("Keyboard interrupt: no data");
    }

    // EOI signal
    outb(PIC1_COMMAND, EOI);
}
