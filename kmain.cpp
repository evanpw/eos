#include "assertions.h"
#include "mem.h"
#include "print.h"
#include "span.h"
#include "stdlib.h"
#include "video.h"

// Should be the only function in this file
extern "C" void kmain() {
    // Print a startup message to the debug console
    println("Kernel started");

    // Fill the screen with green
    clearScreen(0xA0);

    MemoryManager mm;

    println("before:");
    MM.showHeap();

    void* ptr1 = MM.kmalloc(8);
    void* ptr2 = MM.kmalloc(8);
    void* ptr3 = MM.kmalloc(8);
    void* ptr4 = MM.kmalloc(8);

    println("after allocation:");
    MM.showHeap();

    MM.kfree(ptr2);
    MM.kfree(ptr3);

    println("after free:");
    MM.showHeap();

    void* ptr5 = MM.kmalloc(20);

    println("after another alloc:");
    MM.showHeap();
}
