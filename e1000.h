// Driver for an e1000-style ethernet card
#pragma once

#include "address.h"
#include "network_device.h"
#include "pci.h"

class E1000Device : public NicDevice {
public:
    E1000Device();

    void sendPacket(PhysicalAddress buffer, size_t length);
    size_t recvPacket(PhysicalAddress buffer);

private:
    template <typename T>
    static void staticAssert();

    struct RegisterSpace;
    struct TransmitDescriptor;
    struct ReceiveDescriptor;

    PCIDevice* _pciDev;
    RegisterSpace* _regs;
    uint16_t _macAddress[3];
    uint8_t _irqNumber;

    // Transmit descriptor ring
    PhysicalAddress _txDescBase;
    TransmitDescriptor* _txRing;
    size_t _txDescCount;

    // Receive descriptor ring
    PhysicalAddress _rxDescBase;
    ReceiveDescriptor* _rxRing;
    size_t _rxDescCount;

    void initPCI();
    void resetDevice();
    void initEEPROM();
    void initTxRing();
    void initRxRing();

    uint16_t readEEPROM(uint8_t addr);
};
