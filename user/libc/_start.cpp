#include <unistd.h>

#include "heap.h"

extern "C" void _start() __attribute__((section(".entry")));
extern "C" int main();

void _start() {
    initHeap();

    int result = main();
    _exit(result);
}
