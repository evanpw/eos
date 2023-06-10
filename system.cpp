#include "system.h"

#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "mem.h"
#include "print.h"
#include "screen.h"
#include "terminal.h"
#include "user.h"

static void switchAddressSpace(PhysicalAddress pml4) {
    asm volatile("movq %0, %%cr3" : : "r"(pml4.value) : "memory");
}

[[noreturn]] static void jumpToUser(uint64_t rip, uint64_t rsp) {
    asm volatile(
        "movq %0, %%rsp\n"
        "movq %1, %%rcx\n"
        // Turn off everything except reserved bit 1 and interrupts
        "movq $0x202, %%r11\n"
        "sysretq\n"
        :
        : "r"(rsp), "r"(rip)
        : "rcx", "r11", "memory");

    __builtin_unreachable();
}

using SyscallHandler = int64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t);

int64_t sys_add(int64_t a, int64_t b) { return a + b; }

int64_t sys_print(int64_t arg) {
    println("sys_print: {}", arg);
    return 0;
}

static constexpr uint64_t MAX_SYSCALL_NO = 1;

extern "C" SyscallHandler syscallTable[];

// We don't have static initialization, so this is initialized at startup
SyscallHandler syscallTable[MAX_SYSCALL_NO + 1];

extern "C" [[gnu::naked]] void syscallEntry() {
    // TODO: switch to kernel stack
    // TODO: be careful about interrupts
    asm volatile(
        "push %%rcx\n"  // save caller rip
        "push %%r11\n"  // save caller rflags
        // Check for out-of-range syscall #
        "cmp %[MAX_SYSCALL_NO], %%rax\n"
        "jna 1f\n"
        "mov $-1, %%rax\n"
        "jmp 2f\n"
        "1:\n"
        // syscall uses r10 for arg4 while C uses rcx
        "mov %%r10, %%rcx\n"
        "call *syscallTable(, %%rax, 8)\n"
        "2:\n"
        "pop %%r11\n"  // caller rflags
        "pop %%rcx\n"  // caller rip
        "sysretq\n"
        :
        : [MAX_SYSCALL_NO] "i"(MAX_SYSCALL_NO)
        : "memory");

    __builtin_unreachable();
}

void System::run() {
    System system;

    // Model-Specific Registers (MSRs)
    static constexpr uint64_t IA32_EFER = 0xC0000080;
    static constexpr uint64_t IA32_STAR = 0xC0000081;
    static constexpr uint64_t IA32_LSTAR = 0xC0000082;

    // Set syscall enable bit of the IA32_EFER MSR
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | 1);

    // Set the upper dword of the IA32_STAR MSR to 0x0018'0008. This instructs
    // the CPU to set CS to 0x28 (0x18 + 0x10) and SS to 0x20 (0x18 + 0x08). And
    // it also sets up a later syscall to set CS to 0x08 and SS to 0x10
    uint32_t lowStar = lowBits(rdmsr(IA32_STAR), 32);
    wrmsr(IA32_STAR, concatBits((uint32_t)0x00180008, lowStar));

    // Set up syscall to jump to syscallEntry
    wrmsr(IA32_LSTAR, (uint64_t)&syscallEntry);

    // Create table of syscall handlers
    syscallTable[0] = reinterpret_cast<SyscallHandler>(sys_add);
    syscallTable[1] = reinterpret_cast<SyscallHandler>(sys_print);

    println("Entering ring3");

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();

    // Map the kernel image at the user base
    // TODO: we only need to map the usermode function's code
    userAddressSpace.mapPage(userAddressSpace.userMapBase(), PhysicalAddress(0),
                             1);

    // Allocate virtual memory area for the usermode stack
    VirtualAddress userStackBottom = userAddressSpace.vmalloc(4);
    VirtualAddress userStackTop = userStackBottom + 4 * PAGE_SIZE;

    switchAddressSpace(userAddressSpace.pml4().value);
    jumpToUser(userAddressSpace.userMapBase().value + (uint64_t)userTask,
               userStackTop.value);
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    _terminal = new Terminal(*_keyboard, *_screen);
    installInterrupts();
}
