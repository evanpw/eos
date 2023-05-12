#include "misc.h"
#include "video.h"

void halt() {
   while (true) {
      asm volatile("cli; hlt");
   }
}

void panic() {
    puts("\nKernel panicked!\n");
    halt();
}
