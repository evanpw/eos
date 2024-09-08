#include "thread.h"

#include "boot.h"
#include "estd/new.h"  // IWYU pragma: keep
#include "klibc.h"
#include "page_map.h"
#include "process.h"
#include "string.h"
#include "system.h"
#include "trap.h"

Thread* currentThread;

// Defined in entry.S
extern "C" void switchToUserMode();

Thread::Thread(Process* process, VirtualAddress entryPoint) : process(process) {
    // Allocate a user mode stack
    PhysicalAddress stackBottomUserPhys = System::mm().pageAlloc(4);
    VirtualAddress stackBottomUser = process->addressSpace->vmalloc(4);
    VirtualAddress stackTopUser = stackBottomUser + 4 * PAGE_SIZE;
    process->addressSpace->mapPages(stackBottomUser, stackBottomUserPhys, 4);

    // Allocate a kernel stack
    PhysicalAddress stackBottomPhys = System::mm().pageAlloc(4);
    VirtualAddress stackBottom = System::mm().physicalToVirtual(stackBottomPhys);
    VirtualAddress stackTop = stackBottom + 4 * PAGE_SIZE;

    // Construct an initial stack that looks like a thread returning from a syscall
    uint64_t* stackPtr = stackTop.ptr<uint64_t>();
    stackPtr -= sizeof(TrapRegisters) / sizeof(uint64_t);
    memset(stackPtr, 0, sizeof(TrapRegisters));

    TrapRegisters& regs = *new (stackPtr) TrapRegisters;
    regs.rip = entryPoint.value;
    regs.rspPrev = stackTopUser.value;
    regs.rflags = 0x202;  // IF + reserved bit
    regs.ss = SELECTOR_DATA3;
    regs.cs = SELECTOR_CODE3;

    // On top of that, construct a stack which looks like the one constructed by
    // switchContext, except that the return address is switchToUserMode, which pops all
    // the registers from TrapRegisters and issues a sysret to enter user mode
    *(--stackPtr) = bit_cast<uint64_t>((void*)switchToUserMode);

    // The rest of the regs (rbp, rbx, r12, r13, r14, r15) can be set to zero here
    for (size_t i = 0; i < 6; ++i) {
        *(--stackPtr) = 0;
    }

    kernelStack = stackTop.value;
    rsp = bit_cast<uint64_t>(stackPtr);
}
