// Defines some functions for port I/O and reading and writing MSRs
#pragma once
#include <stddef.h>
#include <stdint.h>

inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port) : "memory");
    return result;
}

inline uint16_t inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "Nd"(port) : "memory");
    return result;
}

inline uint32_t inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a"(result) : "Nd"(port) : "memory");
    return result;
}

inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline void insb(void* dest, uint16_t port, size_t count) {
    asm volatile("rep insb"
                 : "=D"(dest), "=c"(count)
                 : "Nd"(port), "0"(dest), "1"(count)
                 : "memory");
}

inline void insw(void* dest, uint16_t port, size_t count) {
    asm volatile("rep insw"
                 : "=D"(dest), "=c"(count)
                 : "Nd"(port), "0"(dest), "1"(count)
                 : "memory");
}

inline void insl(void* dest, uint16_t port, size_t count) {
    asm volatile("rep insl"
                 : "=D"(dest), "=c"(count)
                 : "Nd"(port), "0"(dest), "1"(count)
                 : "memory");
}

inline void iowait(int rounds = 1) {
    for (int i = 0; i < rounds; ++i) {
        // This doesn't do anything, but takes a microsecond or so to do it
        outb(0x80, 0);
    }
}
