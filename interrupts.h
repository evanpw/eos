#pragma once
#include <stdint.h>

#include "estd/bits.h"

// I/O ports for communicating with the PIC
enum : uint16_t {
    PIC1_COMMAND = 0x20,
    PIC1_DATA = 0x21,
    PIC2_COMMAND = 0xA0,
    PIC2_DATA = 0xA1,
};

// End-of-interrupt signal
static constexpr uint8_t EOI = 0x20;

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

struct __attribute__((packed)) TaskStateSegment {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomapOffset;
};

static_assert(sizeof(TaskStateSegment) == 0x68);

void installInterrupts();

enum class InterruptsFlag {
    Enabled = 0,
    Disabled = 1,
};

struct Interrupts {
    static void enable() { asm volatile("sti"); }

    static void disable() { asm volatile("cli"); }

    // Disables interrupts and returns the previous interrupt state
    static InterruptsFlag saveAndDisable() {
        uint64_t rflags;
        asm volatile(
            "pushf\n"
            "pop %0\n"
            : "=rm"(rflags)
            :
            : "memory");

        disable();

        bool ifFlag = rflags & (1 << 9);
        return ifFlag ? InterruptsFlag::Enabled : InterruptsFlag::Disabled;
    }

    // Enables interrupts if they were enabled at the corresponding call to
    // save()
    static void restore(InterruptsFlag state) {
        if (state == InterruptsFlag::Enabled) {
            enable();
        }
    }
};
