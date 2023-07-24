.global syscallEntry

// Defines the syscall entry point to the kernel. Saves registers and routes the syscall
// to the appropriate handler
.global syscallEntryAsm
syscallEntryAsm:
    // TODO: swap to kernel stack and save user stack in non-global location
    mov %rsp, (saveStack)

    // Construct a TrapRegisters struct on the stack
    pushq $0x20        // ss (TODO: make this have the right DPL)
    pushq (saveStack)  // rspPrev
    pushq %r11         // rflags
    pushq $0x28        // cs (TODO: make this have the right DPL)
    pushq %rcx         // rip
    pushq $0           // errorCode
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

    // Pass a pointer to TrapRegisters as the only argument
    mov %rsp, %rdi
    call syscallEntry

    // Return value is placed into TrapRegisters->rax by syscallEntry
.return:
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
    add $8, %rsp // errorCode
    popq %rcx   // rip
    add $8, %rsp // cs
    popq %r11   // rflags
    popq %rsp   // rspPrev (this also skips ss)

    sysretq

saveStack:
    .long 0
