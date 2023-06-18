#include "pci.h"

#include <string.h>

#include "io.h"
#include "print.h"

static constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
static constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

struct __attribute__((packed)) PCIConfigAddress {
    PCIConfigAddress(uint8_t bus, uint8_t device, uint8_t function,
                     uint8_t offset)
    : enable(1),
      reserved(0),
      bus(bus),
      device(device),
      function(function),
      offset(offset) {
        ASSERT(lowBits(offset, 2) == 0);
    }

    // From low to high bits to convince gcc to order them correctly
    uint8_t offset : 8;
    uint8_t function : 3;
    uint8_t device : 5;
    uint8_t bus : 8;
    uint8_t reserved : 7;
    uint8_t enable : 1;

    operator uint32_t() const {
        uint32_t result;
        memcpy(&result, this, 4);
        return result;
    }
};

static_assert(sizeof(PCIConfigAddress) == sizeof(uint32_t));

uint32_t PCIDevice::readConfigDword(uint8_t offset) const {
    // Must be dword-aligned
    ASSERT(lowBits(offset, 2) == 0);

    // Tell the PCI controller which dword to read and then read it
    PCIConfigAddress address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t PCIDevice::readConfigWord(uint8_t offset) const {
    // Must be word-aligned
    ASSERT(lowBits(offset, 1) == 0);

    // Round down the address to be dword-aligned
    uint8_t dwordOffset = clearLowBits(offset, 2);
    uint8_t offsetRemainder = offset - dwordOffset;

    uint32_t result32 = readConfigDword(dwordOffset);

    // Extract the correct word out of the 32-bit result
    return bitRange(result32, 8 * offsetRemainder, 16);
}

uint8_t PCIDevice::readConfigByte(uint8_t offset) const {
    // Round down the address to be dword-aligned
    uint8_t dwordOffset = clearLowBits(offset, 2);
    uint8_t offsetRemainder = offset - dwordOffset;

    uint32_t result32 = readConfigDword(dwordOffset);

    // Extract the correct word out of the 32-bit result
    return bitRange(result32, 8 * offsetRemainder, 8);
}

PCIDevice* g_ideController;

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    if (!PCIDevice::exists(bus, device, function)) return;

    print("{:02d}:{:02d}.{} - ", bus, device, function);

    PCIDevice pciDevice{bus, device, function};
    if (pciDevice.classSubclass() == 0x0101) {
        println("IDE interface");
        g_ideController = new PCIDevice{bus, device, function};
    } else if (pciDevice.classSubclass() == 0x0600) {
        println("Host bridge");
    } else if (pciDevice.classSubclass() == 0x0601) {
        println("ISA bridge");
    } else if (pciDevice.classSubclass() == 0x0300) {
        println("VGA compatible controller");
    } else if (pciDevice.classSubclass() == 0x0200) {
        println("Ethernet controller");
    } else if (pciDevice.classSubclass() == 0x0680) {
        println("Bridge");
    } else {
        println("Unknown Device ({:02X}:{:02X})", pciDevice.classCode(),
                pciDevice.subclass());
    }

    /*
    println("Vendor ID = {:04X}", pciDevice.vendorId());
    println("Device ID = {:04X}", pciDevice.deviceId());
    println("Command = {:04X}", pciDevice.commandWord());
    println("Status = {:04X}", pciDevice.statusWord());
    println("Revision ID = {:02X}", pciDevice.revisionId());
    println("Prog IF = {:02X}", pciDevice.progIf());
    println("Header Type = {:02X}", pciDevice.headerType());
    println("Multifunction = {}", pciDevice.multifunction());
    println("BIST = {:02X}", pciDevice.bist());
    if (pciDevice.headerType() == 0x00) {
        println("BAR0 = {:08X}", pciDevice.bar0());
        println("BAR1 = {:08X}", pciDevice.bar1());
        println("BAR2 = {:08X}", pciDevice.bar2());
        println("BAR3 = {:08X}", pciDevice.bar3());
        println("BAR4 = {:08X}", pciDevice.bar4());
        println("BAR5 = {:08X}", pciDevice.bar5());
        println("Cardbus CIS Pointer = {:08X}", pciDevice.cardbusCISPointer());
        println("Subsystem Vendor ID = {:04X}", pciDevice.subsystemVendorId());
        println("Subsystem ID = {:04X}", pciDevice.subsystemId());
        println("Expansion ROM Base Address = {:08X}",
                pciDevice.expansionROMBaseAddress());
        println("Capabilities Pointer = {:02X}",
                pciDevice.capabilitiesPointer());
        println("Interrupt Line = {:02X}", pciDevice.interruptLine());
        println("Interrupt PIN = {:02X}", pciDevice.interruptPin());
        println("Min Grant = {:02X}", pciDevice.minGrant());
        println("Max Latency = {:02X}", pciDevice.maxLatency());
    }
    */
}

void checkDevice(uint16_t bus, uint8_t device) {
    if (!PCIDevice::exists(bus, device)) return;

    checkFunction(bus, device, 0);

    if (PCIDevice{bus, device}.multifunction()) {
        for (uint8_t function = 1; function < 8; ++function) {
            checkFunction(bus, device, function);
        }
    }
}

void scanBus(uint16_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
        checkDevice(bus, device);
    }
}

void findAllDevices() {
    // bus 0, device 0 will be the Host Bridge
    if (!PCIDevice::exists(0, 0)) {
        println("No PCI bus found");
        return;
    }

    scanBus(0);

    // Multiple PCI controllers
    if (PCIDevice{0, 0}.multifunction()) {
        for (uint8_t bus = 1; bus < 32; ++bus) {
            if (PCIDevice::exists(0, 0, bus)) {
                scanBus(bus);
            }
        }
    }

    // TODO: handle PCI-to-PCI bridges
}

void initPCI() {
    println("Detecting PCI devices");
    findAllDevices();
    println("PCI bus initialized");
}
