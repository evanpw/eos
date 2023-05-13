#include "assertions.h"
#include "print.h"

void halt() {
   while (true) {
      asm volatile("cli; hlt");
   }
}

void panic() {
    println("\nKernel panicked!\n");
    halt();
}
