#include "system.h"

#include "acpi.h"
#include "boot.h"
#include "estd/print.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "klibc.h"
#include "mem.h"
#include "panic.h"
#include "pci.h"
#include "process.h"
#include "processor.h"
#include "screen.h"
#include "string.h"
#include "syscalls.h"
#include "terminal.h"
#include "thread.h"
#include "timer.h"

// Context switches only occur in kernel mode (during a syscall or an interrupt)
// We save the usermode state when entering kernel mode (syscall or interrupt),
// so that's where we return to. Segment registers aren't used, so we don't
// actually need to save them
//
// If our kernel is not preemptable, then we can only switch tasks when exiting
// the kernel, or when the kernel voluntarily gives up control (like a blocking
// syscall)
//
//
// Case 1: timer interrupt while running in ring3: immediately switch
// Case 2: timer interrupt while running in ring0: switch when returning to
// ring3 Case 3: voluntary preemption while running a syscall: immediately
// switch

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

    Process process1{1};
    process1.open(*system._terminal);  // stdin
    process1.open(*system._terminal);  // stdout
    process1.open(*system._terminal);  // stderr

    Thread thread{process1};
    Thread::s_current = &thread;

    // Look up the userland executable on disk
    auto inode = system._fs->lookup("shell.bin");
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    uint64_t pagesNeeded = ceilDiv(inode->size(), PAGE_SIZE);
    PhysicalAddress userDest = mm().pageAlloc(pagesNeeded);
    uint8_t* ptr = mm().physicalToVirtual(userDest).ptr<uint8_t>();

    // Read the executable from disk
    if (!system._fs->readFile(ptr, *inode)) {
        panic("failed to read shell.bin");
    }

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();

    // Map the userland image at the user base
    for (size_t i = 0; i < pagesNeeded; ++i) {
        userAddressSpace.mapPage(userAddressSpace.userMapBase() + i * PAGE_SIZE,
                                 userDest + i * PAGE_SIZE);
    }

    // Allocate a virtual memory area for the usermode stack
    VirtualAddress userStackBottom = userAddressSpace.vmalloc(4);
    VirtualAddress userStackTop = userStackBottom + 4 * PAGE_SIZE;

    println("Entering ring3");
    Processor::loadCR3(userAddressSpace.pml4());
    jumpToUser(userAddressSpace.userMapBase().value, userStackTop.value);
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    Processor::init();
    _screen.assign(new Screen);
    _keyboard.assign(new KeyboardDevice);
    _terminal.assign(new Terminal(*_keyboard, *_screen));
    installInterrupts();
    initSyscalls();
    _pciDevices.assign(new PCIDevices);
    _ideController.assign(new IDEController);
    initACPI();
    Timer::init();

    _fs = Ext2FileSystem::create(_ideController->rootPartition());
    ASSERT(_fs);
}
