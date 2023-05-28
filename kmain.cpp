#include "assertions.h"
#include "interrupts.h"
#include "io.h"
#include "mem.h"
#include "print.h"
#include "span.h"
#include "stdlib.h"
#include "user.h"
#include "video.h"

// Should be the only function in this file
extern "C" void kmain() {
    // Print a startup message to the debug console
    println("Kernel started");

    // Fill the screen with green
    clearScreen(0xA0);

    MemoryManager mm;

    installInterrupts();

    /*
    // Model-Specific Registers (MSRs)
    static constexpr uint64_t IA32_EFER = 0xC0000080;
    static constexpr uint64_t IA32_STAR = 0xC0000081;
    static constexpr uint64_t IA32_LSTAR = 0xC0000082;

    // Set syscall enable bit of the IA32_EFER MSR
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | 1);

    // Set the upper dword of the IA32_STAR MSR to 0x0018'0008. This instructs
    // the CPU to set CS to 0x28 (0x18 + 0x10) and SS to 0x20 (0x18 + 0x08). And
    // it also sets up a later syscall to set CS to 0x08 and SS to 0x10
    uint32_t lowStar = lowBits(rdmsr(IA32_STAR), 32);
    wrmsr(IA32_STAR, concatBits((uint32_t)0x00180008, lowStar));

    asm volatile(
        "mov %0, %%ecx\n"
        // Turn off everything (including interrupts), except reserved bit 1
        "mov $2, %%r11\n"
        "sysretq\n"
        :
        : "i"(userTask)
        : "rcx", "r11", "memory");
    */
}
