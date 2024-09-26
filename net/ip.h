#pragma once
#include <stddef.h>
#include <stdint.h>

class NicDevice;

struct IpAddress {
    uint8_t bytes[4];

    bool operator==(const IpAddress& other) const;

    void print();
};

static_assert(sizeof(IpAddress) == 4);

enum class IpProtocol : uint8_t {
    Icmp = 1,
    Igmp = 2,
    Tcp = 6,
    Udp = 17,
};

class IpHeader {
    uint8_t _headerLen : 4;
    uint8_t _version : 4;
    uint8_t _ecn : 2;
    uint8_t _dscp : 6;
    uint16_t _totalLen;
    uint16_t _identification;
    uint16_t _fragmentOffset : 13;
    uint16_t _flags : 3;
    uint8_t _ttl;
    uint8_t _protocol;
    uint16_t _checksum;
    uint32_t _sourceIp;
    uint32_t _destIp;

public:
    uint8_t headerLen();
    uint8_t version();
    uint8_t ecn();
    uint8_t dscp();
    uint16_t totalLen();
    uint16_t identification();
    uint16_t fragmentOffset();
    uint16_t flags();
    uint8_t ttl();
    IpProtocol protocol();
    uint16_t checksum();
    IpAddress sourceIp();
    IpAddress destIp();

    uint16_t computeChecksum();
};

static_assert(sizeof(IpHeader) == 20);

void ipRecv(NicDevice* nic, uint8_t* buffer, size_t size);
