#pragma once

// Stores the previous register values at kernel entry in a uniform format. We need to
// have the same structure for both IRQs and syscalls because either may cause a thread
// to be rescheduled, so a thread which enters the kernel via one may exit via the other
struct __attribute__((packed)) TrapRegisters {
    // These registers are manually pushed to the stack by the entry code. The ordering
    // of the named (not numbered) registers is the same as the one used by the pusha
    // instruction (if it existed in 64-bit mode), though the order doesn't really matter
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    // Some exceptions push a 64-bit error code after the return address. In all other
    // cases, this will be manually set to zero
    uint64_t errorCode;

    // Hardware-defined interrupt frame. Must be last
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    // If this trap came from usermode, this will point to the user stack, while the rsp
    // member will point to the kernel stack (after switching). But if the trap occured
    // while in kernel mode (an IRQ, for example), this will point to the same stack as
    // rsp (though at a different location)
    uint64_t rspPrev;
    uint64_t ss;
};
