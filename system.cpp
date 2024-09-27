#include "system.h"

#include "acpi.h"
#include "e1000.h"
#include "fs/ext2.h"
#include "ide.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mem.h"
#include "net/arp.h"
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

void System::run() {
    System system;

    // Look up gateway MAC address
    // arpRequest(system._nic.get(), IpAddress(10, 0, 2, 2));

    /*
    // Send a request to google.com
    IpAddress googleIp(142, 250, 80, 46);

    const char* payload = "GET / HTTP/1.1\r\n\r\n";
    size_t packetSize = sizeof(TcpHeader) + strlen(payload);
    uint8_t* packet = new uint8_t[packetSize];

    TcpHeader* tcpHeader = reinterpret_cast<TcpHeader*>(packet);
    tcpHeader->setSourcePort(12345);
    tcpHeader->setDestPort(80);
    tcpHeader->setSeqNum(0);
    tcpHeader->setAckNum(0);
    tcpHeader->setDataOffset(5);
    tcpHeader->setSyn();
    tcpHeader->setWindowSize(64 * KiB - 1);
    memcpy(tcpHeader->data(), payload, strlen(payload));
    tcpHeader->fillChecksum(googleIp, system._nic->ipAddress(), packetSize);

    ipSend(system._nic.get(), googleIp, IpProtocol::Tcp, packet,
           sizeof(TcpHeader) + strlen(payload));
    */

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
    arpInit();
    tcpInit();
    _nic.assign(new E1000Device);
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
