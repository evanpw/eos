#include "scheduler.h"

#include "estd/print.h"
#include "process.h"
#include "processor.h"
#include "thread.h"

uint64_t currentKernelStack;

extern "C" void enterContextImpl(Thread* toThread) {
    currentThread = toThread;
    currentKernelStack = toThread->kernelStack;
    Processor::loadCR3(toThread->process->addressSpace->pml4());
    // TODO: save / restore FPU state
}

// Figure out how to put an initial thread in a state that can be switched to using
// switchContext.
// Needs:
// * thread->rsp set to a stack
// * initial stack has r15, r14, r13, r12, rbx, rbp values on it, followed by an
//   address to continue execution from
// * What address? Needs to enter user mode at the appropriate entry point, so
//   switchToUserMode, except that it needs to have the stack already set up rather
//   than passed in as an argument
//
//  Think of switchContext as a function call which may take a long time to return, but
//  otherwise acts like a normal function. Needs to preserve callee-saved registers, FPU
//  state, address space, etc., but does not need to preserve all registers

extern "C" [[noreturn]] void __attribute__((naked)) enterContext(Thread* toThread) {
    asm volatile(
        "mov %[toRsp], %%rsp\n"
        "call enterContextImpl\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%rbx\n"
        "pop %%rbp\n"
        "ret\n"
        :
        : [toRsp] "m"(toThread->rsp)
        :);
}

void __attribute__((naked)) switchContext(Thread* toThread, Thread* fromThread) {
    asm volatile(
        "push %%rbp\n"
        "mov %%rsp, %%rbp\n"
        "push %%rbx\n"
        "push %%r12\n"
        "push %%r13\n"
        "push %%r14\n"
        "push %%r15\n"
        "mov %%rsp, %[fromRsp]\n"
        "jmp enterContext\n"
        : [fromRsp] "=m"(fromThread->rsp)
        :
        :);
}

void Scheduler::start(Thread* initialThread) {
    Processor::disableInterrupts();
    running = true;
    enterContext(initialThread);
}

void Scheduler::run() {
    if (!running || threads.empty()) return;

    Thread* fromThread = currentThread;
    currentIdx = (currentIdx + 1) % threads.size();
    Thread* toThread = threads[currentIdx];
    switchContext(toThread, fromThread);
}
