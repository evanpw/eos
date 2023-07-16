#include "ide.h"

#include <string.h>

#include "boot.h"
#include "estd/buffer.h"
#include "estd/print.h"
#include "io.h"
#include "pci.h"
#include "system.h"
#include "units.h"

// ATA-6 spec:
// https://web.archive.org/web/20110915154404/http://www.t13.org/Documents/UploadedDocuments/project/d1410r3b-ATA-ATAPI-6.pdf

// Port numbers
enum Register {
    // Command block registers
    Data,
    Error,
    Features,
    SectorCount,
    LBA0,         // aka LBA Low register
    LBA1,         // aka LBA Mid register
    LBA2,         // aka LBA High register
    DriveSelect,  // aka Device register
    Command,
    Status,

    // Control block register
    AltStatus,
    Control,
    REGISTER_COUNT,
};

enum CommandCode : uint8_t {
    ReadPIO = 0x20,
    ReadPIOExt = 0x24,
    ReadDMA = 0xC8,
    ReadDMAExt = 0x25,
    WritePIO = 0x30,
    WritePIOExt = 0x34,
    WriteDMA = 0xCA,
    WriteDMAExt = 0x35,
    CacheFlush = 0xE7,
    CacheFlushExt = 0xEA,
    Packet = 0xA0,
    IdentifyPacket = 0xA1,
    Identify = 0xEC,
};

class StatusFlags {
public:
    StatusFlags(uint8_t raw = 0) : _raw(raw) {}

    bool error() const { return bitRange(_raw, 0, 1); }
    bool dataRequestReady() const { return bitRange(_raw, 3, 1); }
    bool driveFault() const { return bitRange(_raw, 5, 1); }
    bool ready() const { return bitRange(_raw, 6, 1); }
    bool busy() const { return bitRange(_raw, 7, 1); }

    operator uint8_t() const { return _raw; }

private:
    uint8_t _raw;
};

enum class DriveSelector {
    Master,
    Slave,
};

class IDEChannel {
public:
    IDEChannel(uint16_t commandBlock, uint16_t controlBlock);

    uint8_t readRegister(Register reg);
    StatusFlags readStatus();
    StatusFlags readAltStatus();
    uint32_t readSignature();
    uint16_t readData();
    bool isIdle();

    void writeRegister(Register reg, uint8_t value);
    void selectDrive(DriveSelector drive, bool lba = false);
    void sendCommand(CommandCode command);

    void readSector(void* dest);

    // Delay for ~400ns by reading from AltStatus several times
    void delay();

    // Wait until data is available or an error occurs. Returns true if
    // successful
    bool waitForData();

private:
    // Indexed by Register enum
    uint16_t _ports[REGISTER_COUNT];
};

IDEChannel::IDEChannel(uint16_t commandBlock, uint16_t controlBlock) {
    _ports[Data] = commandBlock;
    _ports[Error] = commandBlock + 1;
    _ports[Features] = commandBlock + 1;
    _ports[SectorCount] = commandBlock + 2;
    _ports[LBA0] = commandBlock + 3;
    _ports[LBA1] = commandBlock + 4;
    _ports[LBA2] = commandBlock + 5;
    _ports[DriveSelect] = commandBlock + 6;
    _ports[Command] = commandBlock + 7;
    _ports[Status] = commandBlock + 7;
    _ports[AltStatus] = controlBlock + 2;
    _ports[Control] = controlBlock + 2;
}

uint8_t IDEChannel::readRegister(Register reg) { return inb(_ports[reg]); }

void IDEChannel::writeRegister(Register reg, uint8_t value) {
    outb(_ports[reg], value);
}

StatusFlags IDEChannel::readStatus() { return readRegister(Status); }

StatusFlags IDEChannel::readAltStatus() { return readRegister(AltStatus); }

uint32_t IDEChannel::readSignature() {
    uint8_t byte0 = readRegister(SectorCount);
    uint8_t byte1 = readRegister(LBA0);
    uint8_t byte2 = readRegister(LBA1);
    uint8_t byte3 = readRegister(LBA2);

    return (uint32_t(byte3) << 24) | (uint32_t(byte2) << 16) |
           (uint32_t(byte1) << 8) | byte0;
}

uint16_t IDEChannel::readData() {
    ASSERT(readStatus().dataRequestReady());

    return inw(_ports[Data]);
}

bool IDEChannel::isIdle() {
    StatusFlags status = readStatus();
    return !status.busy() && !status.dataRequestReady();
}

void IDEChannel::readSector(void* dest) {
    ASSERT(readStatus().dataRequestReady());
    insw(dest, _ports[Data], SECTOR_SIZE / 2);
}

void IDEChannel::delay() {
    for (int i = 0; i < 4; ++i) {
        readAltStatus();
    }
}

bool IDEChannel::waitForData() {
    while (true) {
        StatusFlags status = readStatus();

        if (status.busy()) continue;

        if (status.dataRequestReady()) {
            return true;
        }

        if (status.error() || status.driveFault()) {
            return false;
        }
    }
}

void IDEChannel::selectDrive(DriveSelector drive, bool enableLBA) {
    // Bits 5 and 7 are legacy and should always be set
    uint8_t value = (1 << 5) | (1 << 7);

    // Bit 4 selects the drive
    if (drive == DriveSelector::Slave) {
        value |= (1 << 4);
    }

    // Bit 6 enables LBA addressing
    if (enableLBA) {
        value |= (1 << 6);
    }

    writeRegister(DriveSelect, value);
}

void IDEChannel::sendCommand(CommandCode command) {
    writeRegister(Command, command);

    // The spec says that it may take up to 400ns for status to be updated
    delay();
}

bool ATADevice::readSectors(void* dest, uint64_t start, size_t count) {
    // TODO: add support for LBA28
    ASSERT(_lba48 && _channel.isIdle());

    // Enable LBA addressing
    _channel.selectDrive(_drive, true);

    // Write high bytes of parameters
    _channel.writeRegister(SectorCount, highBits(count, 8));
    _channel.writeRegister(LBA0, bitRange(start, 24, 8));
    _channel.writeRegister(LBA1, bitRange(start, 32, 8));
    _channel.writeRegister(LBA2, bitRange(start, 40, 8));

    // Write low bytes of parameters
    _channel.writeRegister(SectorCount, lowBits(count, 8));
    _channel.writeRegister(LBA0, bitRange(start, 0, 8));
    _channel.writeRegister(LBA1, bitRange(start, 8, 8));
    _channel.writeRegister(LBA2, bitRange(start, 16, 8));

    // Send the command
    _channel.sendCommand(ReadPIOExt);

    // Each sector is read separately
    uint8_t* ptr = static_cast<uint8_t*>(dest);
    for (size_t sector = 0; sector < count; ++sector) {
        if (!_channel.waitForData()) {
            return false;
        }

        _channel.readSector(ptr);
        ptr += SECTOR_SIZE;
    }

    return true;
}

// Copies a string from the IDE config info into a standard C string, swapping
// bytes where necessary
static void copyString(char* dest, uint16_t* src, size_t numBytes) {
    char* d = dest;
    const uint16_t* s = src;

    ASSERT(numBytes % 2 == 0);
    for (size_t i = 0; i < numBytes; i += 2) {
        uint16_t next = *s++;
        *d++ = highBits(next, 8);
        *d++ = lowBits(next, 8);
    }

    *d = '\0';
}

OwnPtr<IDEDevice> IDEController::detectDrive(IDEChannel& channel,
                                             DriveSelector drive) {
    channel.selectDrive(drive);
    channel.sendCommand(Identify);

    // If Status == 0, then the drive doesn't exist
    if (channel.readStatus() == 0) {
        return {};
    }

    OwnPtr<IDEDevice> device;

    // Otherwise, wait for command to finish or fail
    if (channel.waitForData()) {
        // Success: this is a regular ATA device
        device.assign(new ATADevice(channel, drive));
    } else {
        // Failure: check the signature bytes to see whether this is an ATAPI
        // device
        if (channel.readSignature() != 0xEB140101) {
            println("ide::detectDrive: error while identifying drive");
            return {};
        }

        // If ATAPI, then use the IdentifyPacket command instead
        channel.sendCommand(IdentifyPacket);

        if (!channel.waitForData()) {
            println("ide::detectDrive: error while identifying ATAPI drive");
            return {};
        }

        device.assign(new ATAPIDevice(channel, drive));
    }

    uint16_t deviceInfo[SECTOR_SIZE / 2];
    channel.readSector(deviceInfo);

    // Get model name as a 40-byte ascii string
    copyString(device->_modelName, &deviceInfo[27], 40);

    // I don't intend to support CHS addressing, so fail if LBA isn't supported
    if (!checkBit(deviceInfo[49], 9)) {
        println("ide::detectDrive: drive does not support LBA addressing");
        return {};
    }

    // Check for 48-bit LBA support
    if (checkBit(deviceInfo[83], 10)) {
        device->_lba48 = true;
        memcpy(&device->_numSectors, &deviceInfo[100], 8);
    } else {
        device->_lba48 = false;
        memcpy(&device->_numSectors, &deviceInfo[60], 4);
    }

    return device;
}

struct __attribute__((packed)) PartitionEntry {
    uint8_t status;
    uint8_t startHead;
    uint8_t startSector : 6;
    uint8_t startCylinderHigh : 2;
    uint8_t startCylinderLow;
    uint8_t partitionType;
    uint8_t endHead;
    uint8_t endSector : 6;
    uint8_t endCylinderHigh : 2;
    uint8_t endCylinderLow;
    uint32_t lbaStart;
    uint32_t numSectors;
};

static_assert(sizeof(PartitionEntry) == 16);

void IDEController::detectPartitions(IDEDevice& device) {
    // TODO: support ATAPI devices (though they usually don't have partitions)
    if (device.isATAPI()) return;

    Buffer firstSector(SECTOR_SIZE);
    if (!device.readSectors(firstSector.get(), 0, 1)) {
        println("can't read first sector of disk");
        return;
    }

    PartitionEntry partitionTable[4];
    memcpy(&partitionTable[0], &firstSector[0x1BE], sizeof(partitionTable));

    for (size_t i = 0; i < 4; ++i) {
        auto& entry = partitionTable[i];
        if (entry.partitionType == 0) continue;

        // TODO: support multiple partitions across drives
        ASSERT(!_rootPartition);
        _rootPartition = OwnPtr(
            new DiskPartitionDevice(device, entry.lbaStart, entry.numSectors));
    }
}

IDEController::IDEController() {
    PCIDevice* pciDevice =
        System::pciDevices().findByClass(PCIDeviceClass::StorageIDE);

    if (!pciDevice) {
        println("No IDE controller found");
        return;
    }

    // Verify that the IDE controller is in compatibility mode
    // TODO: allow native mode (not supported by qemu)
    uint8_t progIf = pciDevice->progIf();
    ASSERT((progIf & ((1 << 0) | (1 << 2))) == 0);

    _primary.assign(new IDEChannel(0x1F0, 0x3F4));
    _secondary.assign(new IDEChannel(0x170, 0x374));

    // Turn off IRQs
    _primary->writeRegister(Control, 2);
    _secondary->writeRegister(Control, 2);

    println("Detecting IDE drives");

    // Detect drives
    _primaryMaster = detectDrive(*_primary, DriveSelector::Master);
    if (_primaryMaster) {
        println("Primary master: {}", _primaryMaster->modelName());
        detectPartitions(*_primaryMaster);
    }

    _primarySlave = detectDrive(*_primary, DriveSelector::Slave);
    if (_primarySlave) {
        println("Primary slave: {}", _primarySlave->modelName());
        detectPartitions(*_primarySlave);
    }

    _secondaryMaster = detectDrive(*_secondary, DriveSelector::Master);
    if (_secondaryMaster) {
        println("Secondary master: {}", _secondaryMaster->modelName());
        detectPartitions(*_secondaryMaster);
    }

    _secondarySlave = detectDrive(*_secondary, DriveSelector::Slave);
    if (_secondarySlave) {
        println("Secondary slave: {}", _secondarySlave->modelName());
        detectPartitions(*_secondarySlave);
    }

    println("IDE controller initialized");
}

IDEController::~IDEController() = default;
