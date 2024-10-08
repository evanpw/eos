#include "heap.h"

#include "unistd.h"

char* nextFreeAddress;
char* heapEnd;

void initHeap() {
    size_t initialHeapSize = 1024 * 1024;  // 1 MiB

    nextFreeAddress = (char*)sbrk(0);
    sbrk(initialHeapSize);
    heapEnd = nextFreeAddress + initialHeapSize;
}
