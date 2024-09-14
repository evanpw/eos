#include "system.h"

#include "acpi.h"
#include "estd/print.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mem.h"
#include "pci.h"
#include "process.h"
#include "processor.h"
#include "scheduler.h"
#include "screen.h"
#include "syscalls.h"
#include "terminal.h"
#include "thread.h"
#include "timer.h"

void System::run() {
    System system;

    Process::create("/bin/shell.bin");
    Process::create("/bin/spam.bin");

    println("Entering ring3");
    system._scheduler->start();
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

    ProcessTable::init();
}

SharedPtr<Terminal> System::terminal() {
    ASSERT(instance()._terminal);
    return instance()._terminal;
}
