#include "system.h"

#include "acpi.h"
#include "e1000.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "keyboard.h"
#include "klibc.h"
#include "mem.h"
#include "net/arp.h"
#include "net/dhcp.h"
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

void testNetwork() {
    // Wait for DHCP to finish
    while (!System::netif().isConfigured()) {
        System::timer().sleep(10);
    }

    // Send a request to gh.evanpw.com
    IpAddress destIp(185, 199, 110, 153);
    const char* payload =
        "GET /boot.asm HTTP/1.1\r\nHost: gh.evanpw.com\r\nConnection: close\r\n\r\n";

    TcpControlBlock* tcb = tcpConnect(&System::netif(), destIp, 80);
    tcpSend(&System::netif(), tcb, reinterpret_cast<const uint8_t*>(payload),
            strlen(payload));

    System::scheduler().stopThread(currentThread);
}

void System::run() {
    System system;

    auto testThread = Thread::createKernelThread(bit_cast<uint64_t>(&testNetwork));
    system._scheduler->startThread(testThread.get());

    Process::create("/bin/shell", nullptr);
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
    _netif.assign(new E1000Device);
    arpInit();
    tcpInit();
    dhcpInit(_netif.get());
    initACPI();
    _scheduler.assign(new Scheduler);
    _timer.assign(new Timer);

    _fs = Ext2FileSystem::create(_ideController->rootPartition());
    ASSERT(_fs);

    ProcessTable::init();
}

estd::shared_ptr<Terminal> System::terminal() {
    ASSERT(instance()._terminal);
    return instance()._terminal;
}
