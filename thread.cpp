#include "thread.h"

#include "boot.h"
#include "page_map.h"
#include "process.h"
#include "string.h"

Thread* Thread::s_current;

Thread::Thread(Process& process, VirtualAddress entryPoint) : process(process) {
    // Allocate a virtual memory area for the usermode stack
    VirtualAddress stackBottom = process.addressSpace->vmalloc(4);
    stackTop = stackBottom + 4 * PAGE_SIZE;

    memset(&regs, 0, sizeof(regs));
    regs.rip = entryPoint.value;
    regs.rspPrev = stackTop.value;
    regs.rflags = 0x202;  // IF + reserved bit
    regs.ss = SELECTOR_DATA3;
    regs.cs = SELECTOR_CODE3;
}
