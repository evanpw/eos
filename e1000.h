// Driver for an e1000-style ethernet card
#pragma once

#include "address.h"
#include "net.h"
#include "network_device.h"
#include "pci.h"

struct TrapRegisters;

class E1000Device : public NicDevice {
public:
    E1000Device();

    const MacAddress& macAddress() const { return _macAddress; }
    void sendPacket(PhysicalAddress buffer, size_t length);
    void flushRx();

    // For use by the irq handler
    uint32_t icr() const;

private:
    template <typename T>
    static void staticAssert();

    struct RegisterSpace;
    struct TransmitDescriptor;
    struct ReceiveDescriptor;

    PCIDevice* _pciDev;
    RegisterSpace* _regs;
    MacAddress _macAddress;
    uint8_t _irqNumber;

    // Transmit descriptor ring
    PhysicalAddress _txDescBase;
    TransmitDescriptor* _txRing;
    size_t _txDescCount;

    // Receive descriptor ring
    PhysicalAddress _rxDescBase;
    ReceiveDescriptor* _rxRing;
    size_t _rxDescCount;

    struct TrapRegisters;
    friend void e1000IrqHandler(TrapRegisters&);

    void initPCI();
    void resetDevice();
    void initEEPROM();
    void initIrq();
    void initTxRing();
    void initRxRing();

    uint16_t readEEPROM(uint8_t addr);
};
