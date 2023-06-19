#pragma once
#include <stdint.h>

#include "estd/bits.h"
#include "estd/vector.h"

class PCIDevice {
public:
    uint16_t bus;
    uint8_t device;
    uint8_t function;

    static bool exists(uint16_t bus, uint8_t device, uint8_t function = 0) {
        return PCIDevice{bus, device, function}.vendorId() != 0xFFFF;
    }

    // Applicable to all PCI devices
    uint16_t vendorId() const { return readConfigWord(0x00); }
    uint16_t deviceId() const { return readConfigWord(0x02); }
    uint16_t commandWord() const { return readConfigWord(0x04); }
    uint16_t statusWord() const { return readConfigWord(0x06); }
    uint8_t revisionId() const { return readConfigByte(0x08); }
    uint8_t progIf() const { return readConfigByte(0x09); }
    uint8_t subclass() const { return readConfigByte(0x0A); }
    uint8_t classCode() const { return readConfigByte(0x0B); }
    uint16_t classSubclass() const { return readConfigWord(0x0A); }
    uint8_t cacheLineSize() const { return readConfigByte(0x0C); }
    uint8_t latencyTimer() const { return readConfigByte(0x0D); }
    uint8_t headerType() const { return lowBits(readConfigByte(0x0E), 7); }
    bool multifunction() const {
        return highBits(readConfigByte(0x0E), 1) != 0;
    }
    uint8_t bist() const { return readConfigByte(0x0F); }

    // Applicable to headerType 0x00 (General Device)
    uint32_t bar0() const { return readConfigDword(0x10); }
    uint32_t bar1() const { return readConfigDword(0x14); }
    uint32_t bar2() const { return readConfigDword(0x18); }
    uint32_t bar3() const { return readConfigDword(0x1C); }
    uint32_t bar4() const { return readConfigDword(0x20); }
    uint32_t bar5() const { return readConfigDword(0x24); }
    uint32_t cardbusCISPointer() const { return readConfigDword(0x28); }
    uint16_t subsystemVendorId() const { return readConfigWord(0x2C); }
    uint16_t subsystemId() const { return readConfigWord(0x2E); }
    uint32_t expansionROMBaseAddress() const { return readConfigDword(0x30); }
    uint8_t capabilitiesPointer() const { return readConfigByte(0x34); }
    uint8_t interruptLine() const { return readConfigByte(0x3C); }
    uint8_t interruptPin() const { return readConfigByte(0x3D); }
    uint8_t minGrant() const { return readConfigByte(0x3E); }
    uint8_t maxLatency() const { return readConfigByte(0x3F); }

    // Applicable to headerType 0x01 (PCI-to-PCI Bridge)
    // TODO: fill this in

    // Applicable to headerType 0x02 (PCI-to-Cardbus Bridge)
    // TODO: fill this in

private:
    uint32_t readConfigDword(uint8_t offset) const;
    uint16_t readConfigWord(uint8_t offset) const;
    uint8_t readConfigByte(uint8_t offset) const;
};

class PCIDevices {
public:
    const Vector<PCIDevice*>& devices() { return _devices; }
    PCIDevice* ideController() { return _ideController; }

private:
    friend class System;
    PCIDevices();

    void findAllDevices();
    void scanBus(uint16_t bus);
    void checkDevice(uint16_t bus, uint8_t device);
    void checkFunction(uint8_t bus, uint8_t device, uint8_t function);

    Vector<PCIDevice*> _devices;
    PCIDevice* _ideController = nullptr;
};
