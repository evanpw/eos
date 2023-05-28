#pragma once
#include <stdint.h>

#include "bits.h"

struct __attribute__((packed)) InterruptDescriptor {
    InterruptDescriptor() = default;
    InterruptDescriptor(uint64_t addr, uint8_t flags);

    uint16_t addr0 = 0;
    uint16_t cs = 0;
    uint8_t ist = 0;
    uint8_t flags = 0;
    uint16_t addr1 = 0;
    uint32_t addr2 = 0;
    uint32_t zero = 0;
};

static_assert(sizeof(InterruptDescriptor) == 16);

struct __attribute__((packed)) IDTRegister {
    uint16_t limit;
    uint64_t addr;
};

extern InterruptDescriptor* g_idt;
extern IDTRegister g_idtr;

struct __attribute__((packed)) InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

void installInterrupts();
