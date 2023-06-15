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

static uint16_t readConfigWord(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset) {
    // Offset must be word-aligned
    ASSERT((offset & 1) == 0);

    // Round down the address to be dword-aligned
    uint8_t dwordOffset = clearLowBits(offset, 2);
    uint8_t offsetRemainder = offset - dwordOffset;

    // Tell the PCI controller which dword we want to read from the config data
    PCIConfigAddress address(bus, device, function, dwordOffset);
    outl(PCI_CONFIG_ADDRESS, address);

    // Extract the correct word out of the 32-bit result
    uint32_t result32 = inl(PCI_CONFIG_DATA);
    return bitRange(result32, 8 * offsetRemainder, 16);
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
