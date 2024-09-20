#include "heap.h"

#include "unistd.h"

uint8_t* nextFreeAddress;
uint8_t* heapEnd;

void initHeap() {
    size_t initialHeapSize = 1024 * 1024;  // 1 MiB

    nextFreeAddress = (uint8_t*)sbrk(0);
    sbrk(initialHeapSize);
    heapEnd = nextFreeAddress + initialHeapSize;
}
