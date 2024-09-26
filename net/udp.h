#pragma once
#include <stddef.h>
#include <stdint.h>

class NicDevice;
struct IpAddress;
struct IpHeader;

class UdpHeader {
    uint16_t _sourcePort;
    uint16_t _destPort;
    uint16_t _length;
    uint16_t _checksum;
    uint8_t _data[];

public:
    uint16_t sourcePort();
    void setSourcePort(uint16_t value);

    uint16_t destPort();
    void setDestPort(uint16_t value);

    uint16_t length();
    void setLength(uint16_t value);

    uint16_t checksum();
    void setChecksum(uint16_t value);

    uint8_t* data();
};

static_assert(sizeof(UdpHeader) == 8);

void udpRecv(NicDevice* nic, IpHeader* ipHeader, uint8_t* buffer, size_t size);
void udpSend(NicDevice* nic, IpAddress destIp, uint16_t sourcePort, uint16_t destPort,
             uint8_t* buffer, uint8_t size);
