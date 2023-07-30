#include "system.h"

#include "acpi.h"
#include "boot.h"
#include "estd/print.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "klibc.h"
#include "mem.h"
#include "panic.h"
#include "pci.h"
#include "process.h"
#include "processor.h"
#include "scheduler.h"
#include "screen.h"
#include "string.h"
#include "syscalls.h"
#include "terminal.h"
#include "thread.h"
#include "timer.h"
#include "trap.h"

// Context switches only occur in kernel mode (during a syscall or an interrupt)
// We save the usermode state when entering kernel mode (syscall or interrupt),
// so that's where we return to. Segment registers aren't used, so we don't
// actually need to save them
//
// If our kernel is not preemptable, then we can only switch tasks when exiting
// the kernel, or when the kernel voluntarily gives up control (like a blocking
// syscall)
//
//
// Case 1: timer interrupt while running in ring3: immediately switch
// Case 2: timer interrupt while running in ring0: switch when returning to ring3
// Case 3: voluntary preemption while running a syscall: immediately switch
//
// Because a thread can enter the kernel via a syscall and exit via an IRQ (or
// vice-versa), we need to save state in a uniform trap frame on every kernel entry that
// is agnostic between those two cases
//
// The call that switches context only needs to save/restore callee-saved registers, since
// it's a function call and C++ will save the caller-saved registers for us on the stack
//
// When a thread is in kernel mode, the top of its kernel stack should always be a
// TrapRegisters stack so that it knows how to return to user mode

void System::run() {
    System system;

    Process process1("shell.bin");
    system._scheduler->threads.push_back(process1.thread.get());
    Process process2("spam.bin");
    system._scheduler->threads.push_back(process2.thread.get());

    println("Entering ring3");
    system._scheduler->start(process2.thread.get());
    __builtin_unreachable();
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    Processor::init();
    installInterrupts();
    _screen.assign(new Screen);
    _keyboard.assign(new KeyboardDevice);
    _terminal.assign(new Terminal(*_keyboard, *_screen));
    initSyscalls();
    _pciDevices.assign(new PCIDevices);
    _ideController.assign(new IDEController);
    initACPI();
    _scheduler.assign(new Scheduler);
    Timer::init();

    _fs = Ext2FileSystem::create(_ideController->rootPartition());
    ASSERT(_fs);
}
