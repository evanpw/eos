#include "assertions.h"
#include "interrupts.h"
#include "mem.h"
#include "print.h"
#include "span.h"
#include "stdlib.h"
#include "video.h"

// Should be the only function in this file
extern "C" void kmain() {
    // Print a startup message to the debug console
    println("Kernel started");

    // Fill the screen with green
    clearScreen(0xA0);

    MemoryManager mm;

    installInterrupts();
}
