#include "system.h"

#include "acpi.h"
#include "e1000.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "keyboard.h"
#include "klibc.h"
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

void testNetwork() {
    NetworkInterface* netif = &sys.netif();

    // Wait for DHCP to finish
    while (!netif->isConfigured()) {
        sys.timer().sleep(10);
    }

    // Lookup gh.evanpw.com via DNS
    const char* hostname = "gh.evanpw.com";
    IpAddress destIp = dnsResolve(netif, IpAddress(10, 0, 2, 3), hostname, true);

    // Send an HTTP request for the root file
    const char* request =
        "GET / HTTP/1.1\r\nHost: gh.evanpw.com\r\nConnection: close\r\n\r\n";

    TcpHandle handle = tcpConnect(netif, destIp, 80);
    tcpSend(netif, handle, request, strlen(request), true);

    // Echo the result to the terminal
    char* buffer = new char[1024];
    while (true) {
        ssize_t bytesRead = tcpRecv(netif, handle, buffer, 1023);
        if (bytesRead < 0) {
            println("tcp error");
            break;
        } else if (bytesRead == 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        print(buffer);
    }
    delete[] buffer;

    sys.scheduler().stopThread(currentThread);
}

void System::run() {
    // auto testThread = Thread::createKernelThread(bit_cast<uint64_t>(&testNetwork));
    //_scheduler->startThread(testThread.get());

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
    initACPI();
    _scheduler.assign(new Scheduler);
    _timer.assign(new Timer);

    _fs = Ext2FileSystem::create(_ideController->rootPartition());
    ASSERT(_fs);

    ProcessTable::init();
}

estd::shared_ptr<Terminal> System::terminal() { return _terminal; }
