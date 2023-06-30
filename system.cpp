#include "system.h"

#include "acpi.h"
#include "boot.h"
#include "estd/print.h"
#include "ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "mem.h"
#include "pci.h"
#include "process.h"
#include "processor.h"
#include "screen.h"
#include "string.h"
#include "syscalls.h"
#include "terminal.h"
#include "thread.h"
#include "timer.h"

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

    // Read the 2nd sector of the disk to find the location and size of the
    // userland image
    ASSERT(g_hardDrive);

    uint16_t* diskMap = new uint16_t[256];
    g_hardDrive->readSectors(diskMap, 1, 1);

    uint16_t userDiskOffset = diskMap[2];
    uint16_t userDiskSize = diskMap[3];
    size_t userImageSize = SECTOR_SIZE * userDiskSize;

    // Read userland from the disk into a fresh piece of page-aligned memory
    uint64_t pagesNeeded = (userImageSize + PAGE_SIZE - 1) / PAGE_SIZE;
    PhysicalAddress userDest = mm().pageAlloc(pagesNeeded);
    g_hardDrive->readSectors(mm().physicalToVirtual(userDest), userDiskOffset,
                             userDiskSize);

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();

    // Map the userland image at the user base
    for (size_t i = 0; i < userImageSize; i += PAGE_SIZE) {
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
    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    _terminal = new Terminal(*_keyboard, *_screen);
    installInterrupts();
    initSyscalls();
    _pciDevices = new PCIDevices;
    initIDE();
    initACPI();
    Timer::init();
    initExt2FS();
}
