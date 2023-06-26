#include "syscalls.h"

#include <stdint.h>

#include "api/syscalls.h"
#include "errno.h"
#include "estd/print.h"
#include "file.h"
#include "io.h"
#include "panic.h"
#include "process.h"
#include "processor.h"
#include "thread.h"

using SyscallHandler = int64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t, uint64_t);

ssize_t sys_read(int fd, void* buffer, size_t count) {
    Process& process = Thread::current().process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || process.openFiles[fd] == nullptr) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = description.file;
    return file.read(description, buffer, count);
}

ssize_t sys_write(int fd, const void* buffer, size_t count) {
    Process& process = Thread::current().process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || process.openFiles[fd] == nullptr) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = description.file;
    return file.write(description, buffer, count);
}

pid_t sys_getpid() {
    Process& process = Thread::current().process;
    return process.pid;
}

void sys_exit(int status) {
    // TODO: tear down the process and return to the scheduler
    println("User process exited with status {}", status);
    halt();
}

// We don't have static initialization, so this is initialized at runtime
SyscallHandler syscallTable[MAX_SYSCALL_NO + 1];

extern "C" [[gnu::naked]] void syscallEntry() {
    // TODO: switch to kernel stack
    // TODO: be careful about interrupts
    asm volatile(
        // Save all callee-clobbered registers
        "push %%rcx\n"
        "push %%rdx\n"
        "push %%rsi\n"
        "push %%rdi\n"
        "push %%r8\n"
        "push %%r9\n"
        "push %%r10\n"
        "push %%r11\n"

        // Check for out-of-range syscall #
        "cmp %[MAX_SYSCALL_NO], %%rax\n"
        "jna 1f\n"
        "mov $-1, %%rax\n"
        "jmp 2f\n"

        // Look up the syscall handler and call it
        "1:\n"
        "mov %%r10, %%rcx\n"  // syscall uses r10 for arg4 while C uses rcx
        "call *syscallTable(, %%rax, 8)\n"

        // Restore all callee-clobbered registers
        "2:\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"

        // Loads rip from rcx and rflags from r11
        "sysretq\n"
        :
        : [MAX_SYSCALL_NO] "i"(MAX_SYSCALL_NO)
        : "memory");

    __builtin_unreachable();
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
    Processor::wrmsr(IA32_LSTAR, (uint64_t)&syscallEntry);

    // Create table of syscall handlers
    syscallTable[SYS_read] = reinterpret_cast<SyscallHandler>(sys_read);
    syscallTable[SYS_write] = reinterpret_cast<SyscallHandler>(sys_write);
    syscallTable[SYS_getpid] = reinterpret_cast<SyscallHandler>(sys_getpid);
    syscallTable[SYS_exit] = reinterpret_cast<SyscallHandler>(sys_exit);

    println("Syscalls initialized");
}
