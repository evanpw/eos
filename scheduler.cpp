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
