#include <unistd.h>

#include "heap.h"

extern "C" void _start(int argc, char* argv[]) __attribute__((section(".entry")));
extern "C" int main(int argc, char* argv[]);

void _start(int argc, char* argv[]) {
    initHeap();

    int result = main(argc, argv);
    _exit(result);
}
