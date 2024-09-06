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

SharedPtr<Terminal> System::terminal() {
    ASSERT(instance()._terminal);
    return instance()._terminal;
}
