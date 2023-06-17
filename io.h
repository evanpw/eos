#pragma once
#include <stdint.h>

#include "bits.h"

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

inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline void iowait(int rounds = 1) {
    for (int i = 0; i < rounds; ++i) {
        // This doesn't do anything, but takes a microsecond or so to do it
        outb(0x80, 0);
    }
}

inline uint64_t rdmsr(uint64_t msr) {
    uint32_t low, high;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return concatBits(high, low);
}

inline void wrmsr(uint64_t msr, uint64_t value) {
    uint32_t low = bitSlice(value, 0, 32);
    uint32_t high = bitSlice(value, 32, 64);

    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}
