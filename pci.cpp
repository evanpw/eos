#include "pci.h"

#include <string.h>

#include "estd/print.h"
#include "io.h"

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

void PCIDevices::checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    if (!PCIDevice::exists(bus, device, function)) return;

    print("{:02d}:{:02d}.{} - ", bus, device, function);

    PCIDevice* pciDevice = new PCIDevice{bus, device, function};
    _devices.push_back(pciDevice);

    if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::StorageIDE) {
        println("IDE interface");
    } else if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::BridgeHost) {
        println("Host bridge");
    } else if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::BridgeISA) {
        println("ISA bridge");
    } else if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::DisplayVGA) {
        println("VGA compatible controller");
    } else if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::NetworkEthernet) {
        println("Ethernet controller");
    } else if (pciDevice->classSubclass() == (uint16_t)PCIDeviceClass::BridgeOther) {
        println("Bridge");
    } else {
        println("Unknown Device ({:02X}:{:02X})", pciDevice->classCode(),
                pciDevice->subclass());
    }
}

void PCIDevices::checkDevice(uint16_t bus, uint8_t device) {
    if (!PCIDevice::exists(bus, device)) return;

    checkFunction(bus, device, 0);

    if (PCIDevice{bus, device}.multifunction()) {
        for (uint8_t function = 1; function < 8; ++function) {
            checkFunction(bus, device, function);
        }
    }
}

void PCIDevices::scanBus(uint16_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
        checkDevice(bus, device);
    }
}

void PCIDevices::findAllDevices() {
    println("Detecting PCI devices");

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

PCIDevice* PCIDevices::findByClass(PCIDeviceClass classCode) {
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i]->classSubclass() == (uint16_t)classCode) {
            return _devices[i];
        }
    }

    return nullptr;
}

PCIDevices::PCIDevices() {
    findAllDevices();
    println("PCI devices initialized");
}
