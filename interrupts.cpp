#include "interrupts.h"
#include "assertions.h"
#include "stdlib.h"
#include "bits.h"
#include "print.h"

InterruptDescriptor g_idt[256];
IDTRegister g_idtr;

void defaultInterruptHandler(InterruptFrame* frame) {
    println("INTERRUPT");
    println("rip: 0x{:X}", frame->rip);
    println("cs: 0x{:X}", frame->cs);
    println("rflags: 0x{:X}", frame->rflags);
    println("rsp: 0x{:X}", frame->rsp);
    println("ss: 0x{:X}", frame->ss);
    halt();
}

[[noreturn]] void handleException(uint8_t vector, InterruptFrame* frame, uint64_t errorCode) {
    println("CPU EXCEPTION: 0x{:X}", vector);
    println("errorCode: 0x{:X}", errorCode);
    println("rip: 0x{:X}", frame->rip);
    println("cs: 0x{:X}", frame->cs);
    println("rflags: 0x{:X}", frame->rflags);
    println("rsp: 0x{:X}", frame->rsp);
    println("ss: 0x{:X}", frame->ss);
    halt();
}

#define FOREACH_EXCEPTION \
    X(0) \
    X(1) \
    X(2) \
    X(3) \
    X(4) \
    X(5) \
    X(6) \
    X(7) \
    X(8) \
    X(9) \
    X(10) \
    X(11) \
    X(12) \
    X(13) \
    X(14) \
    X(15) \
    X(16) \
    X(17) \
    X(18) \
    X(19) \
    X(20) \
    X(21) \
    X(22) \
    X(23) \
    X(24) \
    X(25) \
    X(26) \
    X(27) \
    X(28) \
    X(29) \
    X(30) \
    X(31)

#define X(idx) \
    void __attribute__((interrupt)) exceptionHandler##idx(InterruptFrame* frame, uint64_t errorCode) { \
        handleException(idx, frame, errorCode); \
    }
FOREACH_EXCEPTION
#undef X

static uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port) : "memory");
    return result;
}

static void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

void setDescriptor(uint8_t vector, void* handler, uint8_t flags) {
    auto& descriptor = g_idt[vector];
    descriptor.addr0 = lowBits((uint64_t)handler, 16);
    descriptor.cs = 0x08;
    descriptor.ist = 0;
    descriptor.flags = flags;
    descriptor.addr1 = bitRange((uint64_t)handler, 16, 16);
    descriptor.addr2 = bitRange((uint64_t)handler, 32, 32);
    descriptor.zero = 0;
}

void installInterrupts() {
    memset(g_idt, 0, 256 * sizeof(InterruptDescriptor));

    #define X(idx) setDescriptor(idx, (void*)exceptionHandler##idx, 0x8F);
    FOREACH_EXCEPTION
    #undef X

    g_idtr.addr = (uint64_t)&g_idt[0];
    g_idtr.limit = 255;

    // Mask all IRQs from the PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    asm volatile("lidt %0" : : "m"(g_idtr));
    asm volatile("sti");
}
