#include "stdlib.h"

#include "heap.h"
#include "syscall.h"

// Simple bump-pointer allocator with no free
void* malloc(size_t size) {
    if (nextFreeAddress + size > heapEnd) {
        return nullptr;
    }

    void* result = nextFreeAddress;
    nextFreeAddress += size;
    return result;
}

void free(void* ptr) {}

void exit(int status) {
    syscall(SYS_exit, status);
    __builtin_unreachable();
}
