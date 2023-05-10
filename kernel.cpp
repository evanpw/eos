#include "video.h"

// This must be the first function in the file
extern "C" void kmain() {
    // Print a startup message to the debug console
    puts("Kernel started\n");

    // Fill the screen with green
    clearScreen(0xA0);

    char buffer[32];

    uint32_t numEntries = *(uint32_t*)0x1000;
    puts("Available memory:\n");

    uint32_t* smap = (uint32_t*)0x1004;
    for (uint32_t i = 0; i < numEntries; ++i) {
        uint32_t baseL = *smap++;
        uint32_t baseH = *smap++;
        uint64_t base = (uint64_t)baseH << 32 | baseL;
        uint32_t lengthL = *smap++;
        uint32_t lengthH = *smap++;
        uint64_t length = (uint64_t)lengthH << 32 | lengthL;
        uint32_t type = *smap++;
        uint32_t extended = *smap++;
        uint64_t end = base + length - 1;

        if (length == 0 || type != 1)
            continue;

        size_t bufLength;
        ustr(base, buffer, 16);
        bufLength = strlen(buffer);
        pad(bufLength, 16, true);
        puts(buffer);

        ustr(end, buffer, 16);
        puts(":");
        bufLength = strlen(buffer);
        pad(bufLength, 16, true);
        puts(buffer);
        puts("\n");
    }
}
