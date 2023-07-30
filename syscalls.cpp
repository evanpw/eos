#include "syscalls.h"

#include <stdint.h>

#include "api/syscalls.h"
#include "errno.h"
#include "estd/print.h"
#include "file.h"
#include "io.h"
#include "klibc.h"
#include "panic.h"
#include "process.h"
#include "processor.h"
#include "thread.h"
#include "timer.h"
#include "trap.h"

using SyscallHandler = int64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t);

ssize_t sys_read(int fd, void* buffer, size_t count) {
    Process& process = *currentThread->process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || !process.openFiles[fd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = description.file;
    return file.read(description, buffer, count);
}

ssize_t sys_write(int fd, const void* buffer, size_t count) {
    Process& process = *currentThread->process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || !process.openFiles[fd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = description.file;
    return file.write(description, buffer, count);
}

pid_t sys_getpid() {
    Process& process = *currentThread->process;
    return process.pid;
}

void sys_exit(int status) {
    // TODO: tear down the process and return to the scheduler
    println("User process exited with status {}", status);
    halt();
}

int sys_sleep(int ticks) {
    uint64_t end = Timer::tickCount() + ticks;
    while (Timer::tickCount() < end) {
        Processor::pause();
    }

    return 0;
}

// We don't have static initialization, so this is initialized at runtime
SyscallHandler syscallTable[MAX_SYSCALL_NO + 1];

// Defined in entry.S
extern "C" void syscallEntryAsm();

// Called by syscallEntryAsm
extern "C" void syscallEntry(TrapRegisters& regs) {
    if (regs.rax > MAX_SYSCALL_NO) {
        regs.rax = -1;
        return;
    }

    regs.rax =
        syscallTable[regs.rax](regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9);
}

// Model-Specific Registers (MSRs)
static constexpr uint64_t IA32_EFER = 0xC0000080;
static constexpr uint64_t IA32_STAR = 0xC0000081;
static constexpr uint64_t IA32_LSTAR = 0xC0000082;

void initSyscalls() {
    // Set syscall enable bit of the IA32_EFER MSR
    Processor::wrmsr(IA32_EFER, Processor::rdmsr(IA32_EFER) | 1);

    // Set the upper dword of the IA32_STAR MSR to 0x0018'0008. This instructs
    // the CPU to set CS to 0x28 (0x18 + 0x10) and SS to 0x20 (0x18 + 0x08). And
    // it also sets up a later syscall to set CS to 0x08 and SS to 0x10
    uint32_t lowStar = lowBits(Processor::rdmsr(IA32_STAR), 32);
    Processor::wrmsr(IA32_STAR, concatBits((uint32_t)0x00180008, lowStar));

    // Set up syscall to jump to syscallEntry
    Processor::wrmsr(IA32_LSTAR, (uint64_t)&syscallEntryAsm);

    // Create table of syscall handlers
    syscallTable[SYS_read] = bit_cast<SyscallHandler>(sys_read);
    syscallTable[SYS_write] = bit_cast<SyscallHandler>(sys_write);
    syscallTable[SYS_getpid] = bit_cast<SyscallHandler>(sys_getpid);
    syscallTable[SYS_exit] = bit_cast<SyscallHandler>(sys_exit);
    syscallTable[SYS_sleep] = bit_cast<SyscallHandler>(sys_sleep);

    println("Syscalls initialized");
}
