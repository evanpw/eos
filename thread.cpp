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
extern "C" void enterKernelThread();

estd::unique_ptr<Thread> Thread::createUserThread(Process* process,
                                                  VirtualAddress entryPoint) {
    Thread* thread = new Thread;
    thread->process = process;

    // Allocate a user mode stack
    thread->userStackPages = System::mm().pageAlloc(4);
    VirtualAddress stackBottomUser = process->addressSpace->vmalloc(4);
    VirtualAddress stackTopUser = stackBottomUser + 4 * PAGE_SIZE;
    process->addressSpace->mapPages(stackBottomUser, thread->userStackPages, 4);

    // Allocate a kernel stack
    thread->kernelStackPages = System::mm().pageAlloc(4);
    VirtualAddress stackBottom = System::mm().physicalToVirtual(thread->kernelStackPages);
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

    thread->kernelStack = stackTop.value;
    thread->rsp = bit_cast<uint64_t>(stackPtr);

    return estd::unique_ptr<Thread>(thread);
}

estd::unique_ptr<Thread> Thread::createKernelThread(VirtualAddress entryPoint) {
    Thread* thread = new Thread;
    thread->process = nullptr;

    // Allocate a user mode stack
    thread->userStackPages = 0;

    // Allocate a kernel stack
    thread->kernelStackPages = System::mm().pageAlloc(4);
    VirtualAddress stackBottom = System::mm().physicalToVirtual(thread->kernelStackPages);
    VirtualAddress stackTop = stackBottom + 4 * PAGE_SIZE;

    // Construct an initial stack that looks like a thread returning from an interrupt
    uint64_t* stackPtr = stackTop.ptr<uint64_t>();
    stackPtr -= sizeof(TrapRegisters) / sizeof(uint64_t);
    memset(stackPtr, 0, sizeof(TrapRegisters));

    TrapRegisters& regs = *new (stackPtr) TrapRegisters;
    regs.rip = entryPoint.value;
    regs.rspPrev = stackTop.value;
    regs.rflags = 0x202;  // IF + reserved bit
    regs.ss = SELECTOR_DATA0;
    regs.cs = SELECTOR_CODE0;

    // On top of that, construct a stack which looks like the one constructed by
    // switchContext, except that the return address is switchToUserMode, which pops
    // off the (unused) error code and performs an iretq
    *(--stackPtr) = bit_cast<uint64_t>((void*)enterKernelThread);

    // The rest of the regs (rbp, rbx, r12, r13, r14, r15) can be set to zero here
    for (size_t i = 0; i < 6; ++i) {
        *(--stackPtr) = 0;
    }

    thread->kernelStack = stackTop.value;
    thread->rsp = bit_cast<uint64_t>(stackPtr);

    return estd::unique_ptr<Thread>(thread);
}

Thread::~Thread() {
    if (userStackPages != 0) {
        System::mm().pageFree(userStackPages, 4);
    }

    System::mm().pageFree(kernelStackPages, 4);
}
