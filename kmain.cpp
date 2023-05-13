#include "mem.h"
#include "video.h"
#include "print.h"

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
}
