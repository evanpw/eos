#include "video.h"

// This must be the first function in the file
void kmain() {
    // Print a startup message to the debug console
    puts("Kernel started\n");

    // Fill the screen with green
    clearScreen(0xA0);
}
