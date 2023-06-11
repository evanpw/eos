#include "system.h"

#include "boot.h"
#include "interrupts.h"
#include "stdlib.h"
#include "io.h"
#include "keyboard.h"
#include "mem.h"
#include "print.h"
#include "screen.h"
#include "syscalls.h"
#include "terminal.h"
#include "user.h"

static void switchAddressSpace(PhysicalAddress pml4) {
    asm volatile("movq %0, %%cr3" : : "r"(pml4.value) : "memory");
}

[[noreturn]] static void jumpToUser(uint64_t rip, uint64_t rsp) {
    // TODO: be more careful about interrupts
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

    println("Entering ring3");

    // Compute the location and size of the userland image loaded by the
    // bootloader
    uint8_t* userStart = &_kernelEnd;
    uint8_t* userEnd = _kernelStartPtr + *_imageSizePtr;
    size_t length = userEnd - userStart;

    // Copy the userland code and data to a fresh piece of memory so that it'll
    // be page-aligned
    uint64_t pagesNeeded = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    PhysicalAddress userDest = mm().pageAlloc(pagesNeeded);
    memcpy(mm().physicalToVirtual(userDest), userStart, length);

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();

    // Map the userland image at the user base
    for (size_t i = 0; i < length; i += PAGE_SIZE) {
        userAddressSpace.mapPage(userAddressSpace.userMapBase() + i * PAGE_SIZE,
                                 userDest + i * PAGE_SIZE);
    }

    // Allocate a virtual memory area for the usermode stack
    VirtualAddress userStackBottom = userAddressSpace.vmalloc(4);
    VirtualAddress userStackTop = userStackBottom + 4 * PAGE_SIZE;

    switchAddressSpace(userAddressSpace.pml4().value);
    jumpToUser(userAddressSpace.userMapBase().value, userStackTop.value);
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    _terminal = new Terminal(*_keyboard, *_screen);
    installInterrupts();
    initSyscalls();
}
