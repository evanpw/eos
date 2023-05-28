#pragma once
#include <stdint.h>

inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port) : "memory");
    return result;
}

inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline void iowait() {
    // This doesn't do anything, but takes a microsecond or so to do it
    outb(0x80, 0);
}
