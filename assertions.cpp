#include "assertions.h"
#include "print.h"

void halt() {
   while (true) {
      asm volatile("cli; hlt");
   }
}

void panic() {
    println("\nKernel panic\n");
    halt();
}

void panic(const char* msg) {
    print("\nKernel panic: ");
    println(msg);
    halt();
}
