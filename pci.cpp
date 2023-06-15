#include "pci.h"

#include <stdint.h>

#include "bits.h"
#include "io.h"
#include "print.h"
#include "stdlib.h"

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

static uint32_t readConfigDword(uint8_t bus, uint8_t device, uint8_t function,
                                uint8_t offset) {
    // Must be dword-aligned
    ASSERT(lowBits(offset, 2) == 0);

    // Tell the PCI controller which dword to read and then read it
    PCIConfigAddress address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t readConfigWord(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset) {
    // Must be word-aligned
    ASSERT(lowBits(offset, 1) == 0);

    // Round down the address to be dword-aligned
    uint8_t dwordOffset = clearLowBits(offset, 2);
    uint8_t offsetRemainder = offset - dwordOffset;

    uint32_t result32 = readConfigDword(bus, device, function, dwordOffset);

    // Extract the correct word out of the 32-bit result
    return bitRange(result32, 8 * offsetRemainder, 16);
}

static uint8_t readConfigByte(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset) {
    // Round down the address to be dword-aligned
    uint8_t dwordOffset = clearLowBits(offset, 2);
    uint8_t offsetRemainder = offset - dwordOffset;

    uint32_t result32 = readConfigDword(bus, device, function, dwordOffset);

    // Extract the correct word out of the 32-bit result
    return bitRange(result32, 8 * offsetRemainder, 8);
}

void initPCI() {
    for (uint16_t bus = 0; bus < 256; ++bus) {
        for (uint8_t device = 0; device < 32; ++device) {
            uint16_t vendor = readConfigWord(bus, device, 0, 0);
            if (vendor != 0xFFFF) {
                print("PCI device at bus={}, device={}: ", bus, device);

                uint16_t classCode = readConfigWord(bus, device, 0, 0x0A);
                if (classCode == 0x0101) {
                    println("IDE Controller");
                    println("Vendor ID = {:04X}", readConfigWord(bus, device, 0, 0x00));
                    println("Device ID = {:04X}", readConfigWord(bus, device, 0, 0x02));
                    println("Command = {:04X}", readConfigWord(bus, device, 0, 0x04));
                    println("Status = {:04X}", readConfigWord(bus, device, 0, 0x06));
                    println("Revision ID = {:02X}", readConfigByte(bus, device, 0, 0x08));
                    println("Prog IF = {:02X}", readConfigByte(bus, device, 0, 0x09));
                    println("Subclass = {:02X}", readConfigByte(bus, device, 0, 0x0A));
                    println("Class Code = {:02X}", readConfigByte(bus, device, 0, 0x0B));
                    println("Cache Line Size = {:02X}", readConfigByte(bus, device, 0, 0x0C));
                    println("Latency Timer = {:02X}", readConfigByte(bus, device, 0, 0x0D));
                    println("Header Type = {:02X}", readConfigByte(bus, device, 0, 0x0E));
                    println("BIST = {:02X}", readConfigByte(bus, device, 0, 0x0F));
                    println("BAR0 = {:08X}", readConfigDword(bus, device, 0, 0x10));
                    println("BAR1 = {:08X}", readConfigDword(bus, device, 0, 0x14));
                    println("BAR2 = {:08X}", readConfigDword(bus, device, 0, 0x18));
                    println("BAR3 = {:08X}", readConfigDword(bus, device, 0, 0x1C));
                    println("BAR4 = {:08X}", readConfigDword(bus, device, 0, 0x20));
                    println("BAR5 = {:08X}", readConfigDword(bus, device, 0, 0x24));
                    println("Cardbus CIS Pointer = {:08X}", readConfigDword(bus, device, 0, 0x28));
                    println("Subsystem Vendor ID = {:04X}", readConfigWord(bus, device, 0, 0x2C));
                    println("Subsystem ID = {:04X}", readConfigWord(bus, device, 0, 0x2E));
                    println("Expansion ROM Base Address = {:08X}", readConfigDword(bus, device, 0, 0x30));
                    println("Capabilities Pointer = {:02X}", readConfigByte(bus, device, 0, 0x34));
                    println("Interrupt Line = {:02X}", readConfigByte(bus, device, 0, 0x3C));
                    println("Interrupt PIN = {:02X}", readConfigByte(bus, device, 0, 0x3D));
                    println("Min Grant = {:02X}", readConfigByte(bus, device, 0, 0x3E));
                    println("Max Latency = {:02X}", readConfigByte(bus, device, 0, 0x3F));
                } else if (classCode == 0x0600) {
                    println("Host Bridge");
                } else if (classCode == 0x0601) {
                    println("ISA Bridge");
                } else if (classCode == 0x0300) {
                    println("VGA Compatible Controller");
                } else if (classCode == 0x0200) {
                    println("Ethernet Controller");
                } else {
                    println("Unknown Device ({:04X})", classCode);
                }

            }
        }
    }
}
