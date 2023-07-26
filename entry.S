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
    // Construct a TrapRegisters struct on the stack
    pushq $0x23        // ss
    pushq %rsp         // rspPrev
    addq $8, (%rsp)
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
    popq %rcx   // rip
    add $8, %rsp // cs
    popq %r11   // rflags
    popq %rsp   // rspPrev (this also skips ss)

    sysretq

// Defines the irq entry point to the kernel. Saves registers (beyond those saved by
// hardware), and then passes the interrupt vector and the TrapRegisters struct to the
// C++ entry point
.macro makeIrqEntryAsm idx
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
    makeIrqEntryAsm %idx
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
