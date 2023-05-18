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

    MemoryManager mm(*(uint32_t*)0x1000, (SMapEntry*)0x1004);

    // At this point, we're ready to implement real memory management and kmalloc
}

