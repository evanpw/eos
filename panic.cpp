#include "panic.h"

#include "estd/print.h"
#include "processor.h"

[[noreturn]] void halt() {
    while (true) {
        Processor::disableInterrupts();
        Processor::halt();
    }
}

[[noreturn]] void panic() {
    println("\nKernel panic\n");
    halt();
}

[[noreturn]] void panic(const char* msg) {
    print("\nKernel panic: ");
    println(msg);
    halt();
}
