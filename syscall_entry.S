// External definitions
#include "api/syscalls.h"
.global syscallTable

.global syscallEntry
syscallEntry:
    // Save all callee-clobbered registers
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11

    // Check for out-of-range syscall #
    cmp MAX_SYSCALL_NO, %rax
    jna .makeCall
    mov $-1, %rax
    jmp .return

    // Look up the syscall handler and call it
.makeCall:
    mov %r10, %rcx  // syscall uses r10 for arg4 while C uses rcx
    call *syscallTable(, %rax, 8)

    // Restore all callee-clobbered registers
.return:
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx

    // Loads rip from rcx and rflags from r11
    sysretq
