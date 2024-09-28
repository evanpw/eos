#pragma once
#include <stddef.h>
#include <stdint.h>

class NetworkInterface;

struct MacAddress {
    uint8_t bytes[6] = {};

    MacAddress() = default;
    MacAddress(uint8_t* bytes);

    MacAddress(const MacAddress& other);
    MacAddress& operator=(const MacAddress& other);

    bool operator==(const MacAddress& other) const;

    static MacAddress broadcast();
    void print();
};

static_assert(sizeof(MacAddress) == 6);

enum class EtherType : uint16_t {
    // Values are in network byte order
    Ipv4 = 0x0008,
    Arp = 0x0608,
};

class EthernetHeader {
    uint8_t _destMac[6];
    uint8_t _srcMac[6];
    uint16_t _etherType;

public:
    MacAddress destMac();
    MacAddress srcMac();
    EtherType etherType();

    void setDestMac(MacAddress value);
    void setSrcMac(MacAddress value);
    void setEtherType(EtherType value);
};

static_assert(sizeof(EthernetHeader) == 14);

void ethRecv(NetworkInterface* netif, uint8_t* buffer, size_t size);
void ethSend(NetworkInterface* netif, MacAddress destMac, EtherType ethType, void* buffer,
             size_t size);
