#include "e1000.h"

#include <string.h>

#include "estd/bits.h"
#include "estd/new.h"  // IWYU pragma: keep
#include "estd/print.h"
#include "io.h"
#include "panic.h"
#include "system.h"

struct E1000Device::RegisterSpace {
    volatile uint32_t ctrl;
    alignas(8) volatile uint32_t status;
    alignas(8) volatile uint32_t eecd;
    volatile uint32_t eerd;
    volatile uint32_t _unused1[0x3A];
    volatile uint32_t rctl;
    volatile uint32_t _unused2[0xBF];
    volatile uint32_t tctl;

    volatile uint32_t _unused3[0x8FF];

    // Receive descriptor ring
    volatile uint32_t rdbal;
    volatile uint32_t rdbah;
    volatile uint32_t rdlen;
    alignas(8) volatile uint32_t rdh;
    alignas(8) volatile uint32_t rdt;

    volatile uint32_t _unused4[0x3F9];

    // Transmit descriptor ring
    volatile uint32_t tdbal;
    volatile uint32_t tdbah;
    volatile uint32_t tdlen;
    alignas(8) volatile uint32_t tdh;
    alignas(8) volatile uint32_t tdt;

    volatile uint32_t _unused5[0x6F9];

    volatile uint32_t ral;
    volatile uint32_t rah;
};

template <>
void E1000Device::staticAssert<E1000Device::RegisterSpace>() {
    static_assert(offsetof(RegisterSpace, ctrl) == 0x00000);
    static_assert(offsetof(RegisterSpace, status) == 0x00008);
    static_assert(offsetof(RegisterSpace, eecd) == 0x00010);
    static_assert(offsetof(RegisterSpace, eerd) == 0x00014);
    static_assert(offsetof(RegisterSpace, rctl) == 0x00100);
    static_assert(offsetof(RegisterSpace, tctl) == 0x00400);

    static_assert(offsetof(RegisterSpace, rdbal) == 0x02800);
    static_assert(offsetof(RegisterSpace, rdbah) == 0x02804);
    static_assert(offsetof(RegisterSpace, rdlen) == 0x02808);
    static_assert(offsetof(RegisterSpace, rdh) == 0x02810);
    static_assert(offsetof(RegisterSpace, rdt) == 0x02818);

    static_assert(offsetof(RegisterSpace, tdbal) == 0x03800);
    static_assert(offsetof(RegisterSpace, tdbah) == 0x03804);
    static_assert(offsetof(RegisterSpace, tdlen) == 0x03808);
    static_assert(offsetof(RegisterSpace, tdh) == 0x03810);
    static_assert(offsetof(RegisterSpace, tdt) == 0x03818);

    static_assert(offsetof(RegisterSpace, ral) == 0x5400);
    static_assert(offsetof(RegisterSpace, rah) == 0x5404);
}

enum : uint32_t {
    CTRL_RST = 1 << 26,

    EECD_SK = 1 << 0,
    EECD_CS = 1 << 1,
    EECD_DI = 1 << 2,
    EECD_EE_REQ = 1 << 6,
    EECD_EE_GNT = 1 << 7,
    EECD_EE_PRES = 1 << 8,

    EERD_START = 1 << 0,
    EERD_DONE = 1 << 4,

    TCTL_EN = 1 << 1,

    RCTL_EN = 1 << 1,
    RCTL_BSIZE_SHIFT = 16,
    RCTL_BSEX = 1 << 25,
    RCTL_BSIZE_4096 = (3 << RCTL_BSIZE_SHIFT) | RCTL_BSEX,
    RCTL_BAM = 1 << 15,
};

// Legacy transmit descriptor layout
struct E1000Device::TransmitDescriptor {
public:
    PhysicalAddress bufferAddress;
    uint64_t flags = 0;

    void clearFlags() { flags = 0; }
    void setLength(uint16_t value) { flags = setBitRange(flags, 0, 16, value); }
    void setEndOfPacket() { setCmd(setBit(cmd(), 0)); }
    void setReportStatus() { setCmd(setBit(cmd(), 3)); }

    bool descriptorDone() { return checkBit(status(), 0); }
    bool excessCollisions() { return checkBit(status(), 1); }
    bool lateCollision() { return checkBit(status(), 2); }

private:
    void setCmd(uint8_t value) { flags = setBitRange(flags, 24, 8, value); }

    uint16_t length() { return lowBits(flags, 16); }
    uint8_t cmd() { return bitRange(flags, 24, 8); }
    uint8_t status() { return bitRange(flags, 32, 4); }

    friend void E1000Device::staticAssert<E1000Device::TransmitDescriptor>();
};

template <>
void E1000Device::staticAssert<E1000Device::TransmitDescriptor>() {
    static_assert(sizeof(TransmitDescriptor) == 16);
    static_assert(offsetof(TransmitDescriptor, bufferAddress) == 0x00);
    static_assert(offsetof(TransmitDescriptor, flags) == 0x08);
}

struct E1000Device::ReceiveDescriptor {
public:
    PhysicalAddress bufferAddress;
    uint64_t flags = 0;

    void clearFlags() { flags = 0; }
    void clearStatus() { flags = clearBitRange(flags, 32, 8); }

    uint16_t length() { return lowBits(flags, 16); }
    uint8_t errors() { return bitRange(flags, 40, 8); }
    bool descriptorDone() { return checkBit(status(), 0); }
    bool endOfPacket() { return checkBit(status(), 1); }

private:
    uint8_t status() { return bitRange(flags, 32, 8); }

    friend void E1000Device::staticAssert<E1000Device::ReceiveDescriptor>();
};

template <>
void E1000Device::staticAssert<E1000Device::ReceiveDescriptor>() {
    static_assert(sizeof(ReceiveDescriptor) == 16);
    static_assert(offsetof(ReceiveDescriptor, bufferAddress) == 0x00);
    static_assert(offsetof(ReceiveDescriptor, flags) == 0x08);
}

E1000Device::E1000Device() {
    _pciDev = System::pciDevices().findByClass(PCIDeviceClass::NetworkEthernet);

    // For now, only target the 82540EM-A, though the driver should work with other
    // related Intel devices as well
    if (!_pciDev || !(_pciDev->vendorId() == 0x8086 && _pciDev->deviceId() == 0x100E)) {
        panic("e1000: device not found");
    }

    println("e1000: found device: Intel 82540EM");

    initPCI();
    resetDevice();
    initEEPROM();

    println("e1000: mac address: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
            lowBits(_macAddress[0], 8), highBits(_macAddress[0], 8),
            lowBits(_macAddress[1], 8), highBits(_macAddress[1], 8),
            lowBits(_macAddress[2], 8), highBits(_macAddress[2], 8));

    initTxRing();
    initRxRing();
    println("e1000: init complete");
}

void E1000Device::initPCI() {
    _irqNumber = _pciDev->interruptLine();

    // Enable bus mastering so that the device can perform DMA
    _pciDev->enableBusMastering();

    // The 82540EM supports only 32-bit BARs, and the version emulated by qemu does
    // not support flash memory, so the I/O address is at BAR1 instead of BAR0
    uint32_t bar0 = _pciDev->bar0();
    ASSERT(bitRange(bar0, 0, 1) == 0);  // memory space
    ASSERT(bitRange(bar0, 1, 2) == 0);  // 32-bit address
    ASSERT(bitRange(bar0, 3, 1) == 0);  // non-prefetchable
    PhysicalAddress memBase = clearLowBits(bar0, 4);

    // The register space of the device is mapped to physical memory, so we overlay a
    // struct on top of it to access the registers
    void* memBaseAddr = System::mm().physicalToVirtual(memBase);
    _regs = new (memBaseAddr) RegisterSpace;
}

void E1000Device::resetDevice() {
    // Reset the device by setting the CTRL.RST bit and waiting for it to clear
    _regs->ctrl = _regs->ctrl | CTRL_RST;
    while (_regs->ctrl & CTRL_RST) {
        iowait();
        // TODO: add a timeout
    }
}

void E1000Device::initEEPROM() {
    if (!(_regs->eecd & EECD_EE_PRES)) {
        panic("e1000: eeprom not present");
    }

    // Read the MAC address from EEPROM
    _macAddress[0] = readEEPROM(0);
    _macAddress[1] = readEEPROM(1);
    _macAddress[2] = readEEPROM(2);
}

uint16_t E1000Device::readEEPROM(uint8_t addr) {
    // Write the EEPROM address to the Read Address (EERD.ADDR) field, and simultaneously
    // set the EERD.START bit to start the read
    _regs->eerd = ((uint32_t)addr << 8) | EERD_START;

    // Wait until the Read Done (EERD.DONE) bit is set, and then read the data from the
    // Read Data (EERD.DATA) field
    while (true) {
        uint32_t eerd = _regs->eerd;
        if (eerd & EERD_DONE) {
            // And then read the data from the Read Data (EERD.DATA) field
            return bitRange(eerd, 16, 16);
        }

        iowait();
        // TODO: add a timeout
    }
}

void E1000Device::initTxRing() {
    // TODO: make sure this is in DMA-available memory
    _txDescBase = System::mm().pageAlloc(1);

    // Default-construct the descriptors and get a pointer to the tx ring
    void* txDescAddr = System::mm().physicalToVirtual(_txDescBase);
    _txDescCount = PAGE_SIZE / sizeof(TransmitDescriptor);
    _txRing = new (txDescAddr) TransmitDescriptor[_txDescCount];

    // Tell the device the location and size of the tx ring
    _regs->tdbal = lowBits(_txDescBase.value, 32);
    _regs->tdbah = highBits(_txDescBase.value, 32);
    _regs->tdlen = _txDescCount * sizeof(TransmitDescriptor);
    _regs->tdh = 0;
    _regs->tdt = 0;

    // Enable transmission
    _regs->tctl = _regs->tctl | TCTL_EN;

    // Create a fake test packet to send
    PhysicalAddress packetBase = System::mm().pageAlloc(1);
    void* packetAddr = System::mm().physicalToVirtual(packetBase);
    memcpy(packetAddr, "hello, world!", 13);
    sendPacket(packetBase, 13);
}

void E1000Device::sendPacket(PhysicalAddress buffer, size_t length) {
    size_t idx = _regs->tdt;
    size_t nextIdx = (idx + 1) % _txDescCount;

    // Wait for at least one descriptor to become available
    while (_regs->tdh == nextIdx) {
        iowait();
    }

    // Set up the next descriptor in the ring to point to the packet
    _txRing[idx].bufferAddress = buffer;
    _txRing[idx].clearFlags();
    _txRing[idx].setLength(length);
    _txRing[idx].setEndOfPacket();
    _txRing[idx].setReportStatus();

    // Increment the tail pointer to tell the device to start transmitting
    _regs->tdt = nextIdx;

    while (_regs->tdh != _regs->tdt) {
        iowait();
        // TODO: add a timeout
    }

    ASSERT(_txRing[idx].descriptorDone());
}

enum class EtherType : uint16_t {
    Ipv4 = 0x0008,
    ARP = 0x0608,
    Ipv6 = 0xDD86,
};

struct __attribute__((packed)) EthernetHeader {
    uint8_t dest[6];
    uint8_t src[6];
    EtherType type;
};

void parsePacket(uint8_t* buffer, size_t size) {
    if (size < sizeof(EthernetHeader)) {
        println("malformed ethernet packet: too short");
        return;
    }

    EthernetHeader* header = reinterpret_cast<EthernetHeader*>(buffer);
    println("src: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", header->src[0],
            header->src[1], header->src[2], header->src[3], header->src[4],
            header->src[5]);
    println("dest: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", header->dest[0],
            header->dest[1], header->dest[2], header->dest[3], header->dest[4],
            header->dest[5]);

    switch (header->type) {
        case EtherType::Ipv4:
            println("type: IPv4");
            break;
        case EtherType::ARP:
            println("type: ARP");
            break;
        case EtherType::Ipv6:
            println("type: IPv6");
            break;
        default:
            println("type: unknown");
            break;
    }
}

void E1000Device::initRxRing() {
    // TODO: make sure this is in DMA-available memory
    _rxDescBase = System::mm().pageAlloc(1);

    // Default-construct the descriptors and get a pointer to the rx ring
    void* rxDescAddr = System::mm().physicalToVirtual(_rxDescBase);
    _rxDescCount = PAGE_SIZE / sizeof(ReceiveDescriptor);
    _rxRing = new (rxDescAddr) ReceiveDescriptor[_rxDescCount];

    // Tell the device the location and size of the rx ring
    _regs->rdbal = lowBits(_rxDescBase.value, 32);
    _regs->rdbah = highBits(_rxDescBase.value, 32);
    _regs->rdlen = _txDescCount * sizeof(TransmitDescriptor);
    _regs->rdh = 0;
    _regs->rdt = 0;

    // Enable receive, set the buffer size, and accept broadcast packets
    _regs->rctl = _regs->rctl | RCTL_EN | RCTL_BSIZE_4096 | RCTL_BAM;

    // Receive a packet for testing purposes
    PhysicalAddress packetBase = System::mm().pageAlloc(1);
    uint8_t* packetAddr = System::mm().physicalToVirtual(packetBase).ptr<uint8_t>();
    size_t size = recvPacket(packetBase);

    parsePacket(packetAddr, size);
}

size_t E1000Device::recvPacket(PhysicalAddress buffer) {
    // Set up the next descriptor in the ring to point to the packet
    _rxRing[0].bufferAddress = buffer;
    _rxRing[0].clearFlags();

    // Increment the tail
    _regs->rdt = 1;

    while (_regs->rdh != _regs->rdt) {
        iowait();
        // TODO: add a timeout
    }

    ASSERT(_rxRing[0].descriptorDone());
    return _rxRing[0].length();
}
