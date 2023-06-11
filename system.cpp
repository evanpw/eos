#include "system.h"

#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "boot.h"
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
                                   uint64_t, uint64_t);

int64_t sys_add(int64_t a, int64_t b) { return a + b; }

int64_t sys_print(int64_t arg) {
    println("sys_print: {}", arg);
    return 0;
}

int64_t sys_add6(int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5, int64_t arg6) {
    return arg1 + arg2 + arg3 + arg4 + arg5 + arg6;
}

static constexpr uint64_t MAX_SYSCALL_NO = 2;

extern "C" SyscallHandler syscallTable[];

// We don't have static initialization, so this is initialized at startup
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
    syscallTable[2] = reinterpret_cast<SyscallHandler>(sys_add6);

    println("Entering ring3");

    // Compute the location and size of the userland image loaded by the bootloader
    uint8_t* userStart = &_kernelEnd;
    uint8_t* userEnd = _kernelStartPtr + *_imageSizePtr;
    size_t length = userEnd - userStart;

    // Copy the userland code and data to a fresh piece of memory so that it'll be page-aligned
    uint64_t pagesNeeded = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    PhysicalAddress userDest = mm().pageAlloc(pagesNeeded);
    uint8_t* src = userStart;
    uint8_t* dest = mm().physicalToVirtual(userDest).ptr<uint8_t>();
    // TODO: use memcpy
    for (size_t i = 0; i < length; ++i) {
        *dest++ = *src++;
    }

    UserAddressSpace userAddressSpace =
        mm().kaddressSpace().makeUserAddressSpace();

    // Map the userland image at the user base
    for (size_t i = 0; i < length; i += PAGE_SIZE) {
        userAddressSpace.mapPage(userAddressSpace.userMapBase() + i * PAGE_SIZE, userDest + i * PAGE_SIZE);
    }

    // Allocate a virtual memory area for the usermode stack
    VirtualAddress userStackBottom = userAddressSpace.vmalloc(4);
    VirtualAddress userStackTop = userStackBottom + 4 * PAGE_SIZE;

    switchAddressSpace(userAddressSpace.pml4().value);
    jumpToUser(userAddressSpace.userMapBase().value, userStackTop.value);
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
