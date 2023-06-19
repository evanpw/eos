// Drivers for an IDE controller connected to the PCI bus and for ATA and ATAPI
// devices connected to it
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/print.h"

class IDEChannel;
enum class DriveSelector;

// A disk device connected to an IDE controller, either standard ATA or ATAPI
struct IDEDevice {
    IDEDevice(IDEChannel& channel, DriveSelector drive)
    : channel(channel), drive(drive) {}

    virtual bool readSectors(void* dest, uint64_t start, size_t count) = 0;

    virtual bool isATAPI() const = 0;

    IDEChannel& channel;
    DriveSelector drive;

    char modelName[41] = {0};
    bool lba48 = false;
    uint64_t numSectors = 0;
};

// A standard (non-packet) ATA device
struct ATADevice : public IDEDevice {
    ATADevice(IDEChannel& channel, DriveSelector drive)
    : IDEDevice(channel, drive) {}

    bool readSectors(void* dest, uint64_t start, size_t count) override;

    bool isATAPI() const override { return false; }
};

// An ATAPI (ATA packet interface) device, usually an optical drive
struct ATAPIDevice : public IDEDevice {
    ATAPIDevice(IDEChannel& channel, DriveSelector drive)
    : IDEDevice(channel, drive) {}

    bool readSectors(void* dest, uint64_t start, size_t count) override {
        println("ATAPIDevice::readSectors not implemented");
        return false;
    }

    bool isATAPI() const override { return true; }
};

// TODO: encapsulate this rather than making it global
extern IDEDevice* g_hardDrive;

void initIDE();
