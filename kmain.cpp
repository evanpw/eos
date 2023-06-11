#include "print.h"
#include "system.h"

extern "C" void kmain() __attribute__((section(".entry")));

// Should be the only function in this file
extern "C" void kmain() {
    println("Kernel started");
    System::run();
}
