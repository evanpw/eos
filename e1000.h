// Driver for an e1000-style ethernet card
#pragma once

#include "address.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/network_interface.h"
#include "pci.h"

struct TrapRegisters;

class E1000Device : public NetworkInterface {
public:
    E1000Device();

    void sendPacket(uint8_t* buffer, size_t length) override;

private:
    template <typename T>
    static void staticAssert();

    struct RegisterSpace;
    struct TransmitDescriptor;
    struct ReceiveDescriptor;

    PCIDevice* _pciDev;
    RegisterSpace* _regs;
    uint8_t _irqNumber;

    // Transmit descriptor ring
    PhysicalAddress _txDescBase;
    TransmitDescriptor* _txRing;
    size_t _txDescCount;

    // Receive descriptor ring
    PhysicalAddress _rxDescBase;
    ReceiveDescriptor* _rxRing;
    size_t _rxDescCount;

    void irqHandler();
    void flushRx();

    void initPCI();
    void resetDevice();
    void initEEPROM();
    void initIrq();
    void initTxRing();
    void initRxRing();

    uint16_t readEEPROM(uint8_t addr);
};
