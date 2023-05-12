#include "mem.h"
#include "video.h"
#include "misc.h"

// Should be the only function in this file
extern "C" void kmain() {
    // Print a startup message to the debug console
    puts("Kernel started\n");

    // Fill the screen with green
    clearScreen(0xA0);

    uint32_t numEntries = *(uint32_t*)0x1000;
    puts("Available memory:\n");

    char buffer[32];
    SMapEntry* entry = (SMapEntry*)0x1004;
    for (uint32_t i = 0; i < numEntries; ++i, ++entry) {
        if (entry->length == 0 || entry->type != 1)
            continue;

        ustr(entry->base, buffer, 16, 8, '0');
        puts(buffer);

        puts(":");

        uint64_t end = entry->base + entry->length - 1;
        ustr(end, buffer, 16, 8, '0');
        puts(buffer);
        puts("\n");
    }
}
