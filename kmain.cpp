#include "mem.h"
#include "video.h"
#include "print.h"
#include "assertions.h"

// Should be the only function in this file
extern "C" void kmain() {
    // Print a startup message to the debug console
    println("Kernel started");

    // Fill the screen with green
    clearScreen(0xA0);

    uint32_t numEntries = *(uint32_t*)0x1000;
    println("Available memory:");

    char buffer[32];
    SMapEntry* entry = (SMapEntry*)0x1004;
    for (uint32_t i = 0; i < numEntries; ++i, ++entry) {
        if (entry->length == 0 || entry->type != 1)
            continue;

        uint64_t end = entry->base + entry->length - 1;
        println("{:08X}:{:08X}", entry->base, end);
    }

    // We want to map all physical memory in virtual memory at the beginning
    // of the top-half of memory
    PhysicalAddress zero(0);
    VirtualAddress pageOffsetBase(0xFFFF800000000000);

    // Attempt to map the first 2MiB of physical memory to high virtual memory
    BootstrapAllocator alloc(1 * MiB, 2 * MiB);
    for (size_t i = 0; i < 512; ++i) {
        mapPage(alloc, pageOffsetBase + i * PAGE_SIZE, zero + i * PAGE_SIZE);
    }

    // At this point, we're ready to implement real memory management and kmalloc
}

