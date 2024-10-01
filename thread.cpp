#include "thread.h"

#include "boot.h"
#include "estd/new.h"  // IWYU pragma: keep
#include "klibc.h"
#include "mm.h"
#include "page_map.h"
#include "process.h"
#include "string.h"
#include "trap.h"

Thread* currentThread;

// Defined in entry.S
extern "C" void switchToUserMode();
extern "C" void enterKernelThread();

estd::unique_ptr<Thread> Thread::createUserThread(Process* process,
                                                  VirtualAddress entryPoint,
                                                  const char* programName,
                                                  const char* argv[]) {
    Thread* thread = new Thread;
    thread->process = process;

    // Allocate and map a user mode stack
    PhysicalAddress userStackBottom = mm.pageAlloc(4);
    VirtualAddress userStackBottomVirt = process->addressSpace->vmalloc(4);

    thread->userStackTop = userStackBottom + 4 * PAGE_SIZE;
    thread->userStackTopVirt = userStackBottomVirt + 4 * PAGE_SIZE;
    thread->userStackPages = 4;

    process->addressSpace->mapPages(userStackBottomVirt, userStackBottom, 4);

    // Allocate a kernel stack
    PhysicalAddress kernelStackBottom = mm.pageAlloc(4);

    thread->kernelStackTop = kernelStackBottom + 4 * PAGE_SIZE;
    thread->kernelStackPages = 4;

    VirtualAddress stackTop = mm.physicalToVirtual(thread->kernelStackTop);

    // Construct an initial stack that looks like a thread returning from a syscall
    uint64_t* stackPtr = stackTop.ptr<uint64_t>();
    stackPtr -= sizeof(TrapRegisters) / sizeof(uint64_t);
    memset(stackPtr, 0, sizeof(TrapRegisters));

    TrapRegisters& regs = *new (stackPtr) TrapRegisters;
    regs.rip = entryPoint.value;
    regs.rspPrev = thread->userStackTopVirt.value;
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

    // Set up the initial user stack to pass arguments to the entry point
    uint8_t* stackPtrK = mm.physicalToVirtual(thread->userStackTop).ptr<uint8_t>();
    VirtualAddress stackPtrU = thread->userStackTopVirt;

    // First count the number of arguments
    int argc = 0;
    if (argv) {
        for (const char** arg = argv; *arg; ++arg) {
            ++argc;
        }
    }

    // Then place a copy of each argument on the stack (and keep track of their
    // location)
    estd::vector<VirtualAddress> userArgv(argc + 2);
    for (int i = argc - 1; i >= 0; --i) {
        const char* arg = argv[i];
        size_t len = strlen(arg) + 1;
        stackPtrK -= len;
        stackPtrU -= len;
        memcpy(stackPtrK, arg, len);
        userArgv[i + 1] = stackPtrU;
    }

    // Also place the program name in the user argv array
    size_t len = strlen(programName) + 1;
    stackPtrK -= len;
    stackPtrU -= len;
    memcpy(stackPtrK, programName, len);
    userArgv[0] = stackPtrU;

    // Then push pointers to each argument on the stack
    stackPtrK -= sizeof(VirtualAddress) * (argc + 2);
    stackPtrU -= sizeof(VirtualAddress) * (argc + 2);
    memcpy(stackPtrK, userArgv.data(), sizeof(VirtualAddress) * (argc + 2));

    // Finally, put argc and the address of the argv array in the appropriate
    // registers to pass them as arguments to _start
    regs.rdi = argc + 1;
    regs.rsi = stackPtrU.value;
    regs.rspPrev = stackPtrU.value;

    return estd::unique_ptr<Thread>(thread);
}

estd::unique_ptr<Thread> Thread::createKernelThread(VirtualAddress entryPoint) {
    Thread* thread = new Thread;
    thread->process = nullptr;

    // Allocate a user mode stack
    thread->userStackTop = 0;

    // Allocate a kernel stack
    PhysicalAddress kernelStackBottom = mm.pageAlloc(4);

    thread->kernelStackTop = kernelStackBottom + 4 * PAGE_SIZE;
    thread->kernelStackPages = 4;

    VirtualAddress stackTop = mm.physicalToVirtual(thread->kernelStackTop);

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
    // Kernel threads have no user stack
    if (userStackTop != 0) {
        mm.pageFree(userStackBottom(), userStackPages);
    }

    // Every thread has a kernel stack
    mm.pageFree(kernelStackBottom(), kernelStackPages);
}
