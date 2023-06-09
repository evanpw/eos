#include "system.h"

#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "mem.h"
#include "print.h"
#include "screen.h"
#include "terminal.h"
#include "user.h"

static void switchAddressSpace(PhysicalAddress pml4) {
    asm volatile("movq %0, %%cr3" : : "r"(pml4.value) : "memory");
}

[[noreturn]] static void jumpToUser(uint64_t rip, uint64_t rsp) {
    asm volatile(
        "movq %0, %%rsp\n"
        "movq %1, %%rcx\n"
        // Turn off everything except reserved bit 1 and interrupts
        "movq $0x202, %%r11\n"
        "sysretq\n"
        :
        : "r"(rsp), "r"(rip)
        : "rcx", "r11", "memory");

    __builtin_unreachable();
}

void System::run() {
    System system;

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

    println("Entering ring3");

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();
    println("pml4={:X}", userAddressSpace.pml4().value);

    // Map the kernel image at the user base
    // TODO: we only need to map the usermode function's code
    userAddressSpace.mapPage(userAddressSpace.userMapBase(), PhysicalAddress(0),
                             1);

    // Allocate virtual memory area for the usermode stack
    VirtualAddress userStackBottom = userAddressSpace.vmalloc(4);
    VirtualAddress userStackTop = userStackBottom + 4 * PAGE_SIZE;

    switchAddressSpace(userAddressSpace.pml4().value);
    jumpToUser(userAddressSpace.userMapBase().value + (uint64_t)userTask,
               userStackTop.value);
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    _terminal = new Terminal(*_keyboard, *_screen);
    installInterrupts();
}
