.altmacro
.global syscallEntry
.global irqEntry

.macro PUSH_GENERAL_REGS
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro POP_GENERAL_REGS
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    add $8, %rsp
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
.endm

// Defines the syscall entry point to the kernel. Saves registers and routes the syscall
// to the appropriate handler
.global syscallEntryAsm
syscallEntryAsm:
    // Interrupts are disabled here, because if an interrupt occurs, it will use the user
    // stack (we're in ring0 now, so the stack isn't automatically switched). We'll enable
    // them again after switching stacks and saving the user stack pointer

    // Save the user-mode stack temporarily, and load the kernel stack
    mov %rsp, [userStack] // TODO: this should be per-CPU
    mov [currentKernelStack], %rsp

    // Construct a TrapRegisters struct on the stack
    pushq $0x23        // ss
    pushq [userStack]  // rspPrev
    sti
    pushq %r11         // rflags
    pushq $0x2B        // cs
    pushq %rcx         // rip
    pushq $0           // errorCode
    PUSH_GENERAL_REGS

    // Pass a pointer to TrapRegisters as the only argument
    mov %rsp, %rdi
    call syscallEntry

    // Return value is placed into TrapRegisters->rax by syscallEntry
    POP_GENERAL_REGS
    add $8, %rsp // errorCode
    popq %rcx    // rip
    add $8, %rsp // cs
    popq %r11    // rflags
    popq %rsp    // rspPrev (this also skips ss)

    sysretq

userStack:
    .quad 0

// Enters a user mode thread for the first time, as if returning from a syscall
.global switchToUserMode
switchToUserMode:
    POP_GENERAL_REGS
    add $8, %rsp // errorCode
    popq %rcx    // rip
    add $8, %rsp // cs
    popq %r11    // rflags
    popq %rsp    // rspPrev (this also skips ss)

    sysretq

// Defines the irq entry point to the kernel. Saves registers (beyond those saved by
// hardware), and then passes the interrupt vector and the TrapRegisters struct to the
// C++ entry point
.macro makeIrqEntry idx
irqEntryAsm\idx:
    // Construct a TrapRegisters struct on the stack. The ss, rsp, rflags, cs, and rip
    // registers are already saved by hardware
    pushq $0 // errorCode
    PUSH_GENERAL_REGS

    // Pass the irq index and the TrapRegisters as arguments to the C++ handler
    mov $\idx, %rdi
    mov %rsp, %rsi
    call irqEntry

    POP_GENERAL_REGS
    add $8, %rsp // errorCode

    iretq
.endm

// Construct the entry points for all 16 IRQs
.set idx,0
.rept 16
    makeIrqEntry %idx
    .set idx,idx+1
.endr

// Construct a table of function pointers to the IRQ entry points
.macro irqLabel idx
    .quad irqEntryAsm\idx
.endm

.global irqEntriesAsm
irqEntriesAsm:
    .set idx,0
    .rept 16
        irqLabel %idx
        .set idx,idx+1
    .endr

// Creates ISR entry point for an exception that does not have an error code
.macro makeExceptionEntry idx
.global exceptionHandler\idx
exceptionEntryAsm\idx:
    pushq $0 // errorCode
    PUSH_GENERAL_REGS

    // Pass the TrapRegisters struct as the only argument to the C++ handler
    mov %rsp, %rdi
    call exceptionHandler\idx

    POP_GENERAL_REGS
    add $8, %rsp // errorCode

    iretq
.endm

// Creates ISR entry point for an exception that _does_ have an error code
.macro makeExceptionEntryWithCode idx
.global exceptionHandler\idx
exceptionEntryAsm\idx:
    PUSH_GENERAL_REGS

    // Pass the TrapRegisters struct as the only argument to the C++ handler
    mov %rsp, %rdi
    call exceptionHandler\idx

    POP_GENERAL_REGS
    iretq
.endm

makeExceptionEntry 0
makeExceptionEntry 1
makeExceptionEntry 2
makeExceptionEntry 3
makeExceptionEntry 4
makeExceptionEntry 5
makeExceptionEntry 6
makeExceptionEntry 7
makeExceptionEntryWithCode 8
makeExceptionEntry 9
makeExceptionEntryWithCode 10
makeExceptionEntryWithCode 11
makeExceptionEntryWithCode 12
makeExceptionEntryWithCode 13
makeExceptionEntryWithCode 14
makeExceptionEntry 15
makeExceptionEntry 16
makeExceptionEntryWithCode 17
makeExceptionEntry 18
makeExceptionEntry 19
makeExceptionEntry 20
makeExceptionEntryWithCode 21
makeExceptionEntry 22
makeExceptionEntry 23
makeExceptionEntry 24
makeExceptionEntry 25
makeExceptionEntry 26
makeExceptionEntry 27
makeExceptionEntry 28
makeExceptionEntryWithCode 29
makeExceptionEntryWithCode 30
makeExceptionEntry 31

// Construct a table of function pointers to the exception entry points
.macro exceptionLabel idx
    .quad exceptionEntryAsm\idx
.endm

.global exceptionEntriesAsm
exceptionEntriesAsm:
    .set idx,0
    .rept 32
        exceptionLabel %idx
        .set idx,idx+1
    .endr
