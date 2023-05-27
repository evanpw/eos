#pragma once
#include <stdint.h>

struct __attribute__((packed)) InterruptDescriptor {
    uint16_t addr0;
    uint16_t cs;
    uint8_t ist;
    uint8_t flags;
    uint16_t addr1;
    uint32_t addr2;
    uint32_t zero;
};

static_assert(sizeof(InterruptDescriptor) == 16);

struct __attribute__((packed)) IDTRegister {
    uint16_t limit;
    uint64_t addr;
};

extern InterruptDescriptor g_idt[256];
extern IDTRegister g_idtr;

struct __attribute__((packed)) InterruptFrame
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

__attribute__((interrupt)) void defaultInterruptHandler(InterruptFrame* frame);

void installInterrupts();
