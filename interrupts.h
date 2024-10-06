// Programs the PIC, sets up an IDT, and defines IRQ and exception handlers
#pragma once
#include <stdint.h>

#include "estd/functional.h"
#include "processor.h"

enum : uint8_t {
    IRQ_TIMER = 0,
    IRQ_KEYBOARD = 1,
    IRQ_MOUSE = 12,
    IRQ_PRIMARY_ATA = 14,
    IRQ_SECONDARY_ATA = 15,
};

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

// Interrupt descriptor flags
static constexpr uint8_t ISR_PRESENT = 0x80;
static constexpr uint8_t ISR_INTERRUPT_GATE = 0x0E;
static constexpr uint8_t ISR_TRAP_GATE = 0x0F;

extern InterruptDescriptor* g_idt;
extern IDTRegister g_idtr;

struct __attribute__((packed)) InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

using IRQHandler = estd::function<void(uint8_t irqNo)>;

void installInterrupts();
void registerIrqHandler(uint8_t irqNo, IRQHandler handler);
bool inIrq();

// For use by the irq handlers to signal the end of interrupt handling
void endOfInterrupt(uint8_t irqNo);
