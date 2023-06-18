#include "panic.h"

#include "estd/print.h"

[[noreturn]] void halt() {
    while (true) {
        asm volatile("cli; hlt");
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
