#include "print.h"
#include "system.h"

// Should be the only function in this file
extern "C" void kmain() {
    println("Kernel started");
    System::run();
}
