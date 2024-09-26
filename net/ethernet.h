#pragma once
#include <stddef.h>
#include <stdint.h>

class NicDevice;

struct MacAddress {
    uint8_t bytes[6];

    static MacAddress broadcast();

    bool operator==(const MacAddress& other) const;

    void print();
};

static_assert(sizeof(MacAddress) == 6);

enum class EtherType : uint16_t {
    Ipv4 = 0x0800,
    Arp = 0x0806,
    Ipv6 = 0x86DD,
};

class EthernetHeader {
    uint8_t _destMac[6];
    uint8_t _srcMac[6];
    uint16_t _etherType;

public:
    MacAddress destMac();
    void setDestMac(MacAddress value);

    MacAddress srcMac();
    void setSrcMac(MacAddress value);

    EtherType etherType();
    void setEtherType(EtherType value);
};

static_assert(sizeof(EthernetHeader) == 14);

void ethRecv(NicDevice* nic, uint8_t* buffer, size_t size);
void ethSend(NicDevice* nic, MacAddress destMac, EtherType ethType, void* buffer,
             size_t size);
