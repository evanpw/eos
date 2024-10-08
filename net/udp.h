#pragma once
#include <stddef.h>
#include <stdint.h>

class NetworkInterface;
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
    uint16_t destPort();
    uint16_t length();
    uint16_t checksum();

    void setSourcePort(uint16_t value);
    void setDestPort(uint16_t value);
    void setLength(uint16_t value);
    void setChecksum(uint16_t value);

    uint8_t* data();
    uint16_t dataLen();
};

static_assert(sizeof(UdpHeader) == 8);

void udpRecv(NetworkInterface* netif, IpHeader* ipHeader, uint8_t* buffer, size_t size);
void udpBroadcast(NetworkInterface* netif, uint16_t sourcePort, uint16_t destPort,
                  uint8_t* buffer, uint8_t size);
void udpSend(IpAddress destIp, uint16_t sourcePort, uint16_t destPort, uint8_t* buffer,
             uint8_t size);
