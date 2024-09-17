#include "estd/print.h"
#include "system.h"

extern "C" void kmain() __attribute__((section(".entry")));

// Should be the only function in this file
extern "C" void kmain() {
    println("boot: kernel started");
    System::run();
}
