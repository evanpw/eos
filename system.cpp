#include "system.h"

#include "acpi.h"
#include "e1000.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mm.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/dns.h"
#include "net/ip.h"
#include "net/tcp.h"
#include "pci.h"
#include "process.h"
#include "processor.h"
#include "scheduler.h"
#include "screen.h"
#include "syscalls.h"
#include "terminal.h"
#include "thread.h"
#include "timer.h"

// Constructed by kmain
System sys;

void System::run() {
    Process::create("/bin/shell", nullptr);
    _scheduler->start();

    __builtin_unreachable();
}

System::System() {
    new (&mm) MemoryManager();

    Processor::init();
    installInterrupts();
    _screen.assign(new Screen);
    _keyboard.assign(new KeyboardDevice);
    _terminal.assign(new Terminal(*_keyboard, *_screen));
    initSyscalls();
    _pciDevices.assign(new PCIDevices);
    _ideController.assign(new IDEController);
    _netif.assign(new E1000Device);
    arpInit();
    tcpInit();
    dhcpInit(_netif.get());
    dnsInit();
    ipInit();
    initACPI();
    _scheduler.assign(new Scheduler);
    _timer.assign(new Timer);

    _fs = Ext2FileSystem::create(_ideController->rootPartition());
    ASSERT(_fs);

    ProcessTable::init();
}

estd::shared_ptr<Terminal> System::terminal() { return _terminal; }
