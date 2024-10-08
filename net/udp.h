#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/stddef.h"

class NetworkInterface;
struct IpAddress;
struct IpHeader;

class UdpHeader {
    uint16_t _sourcePort;
    uint16_t _destPort;
    uint16_t _length;
    uint16_t _checksum;
    byte _data[];

public:
    uint16_t sourcePort();
    uint16_t destPort();
    uint16_t length();
    uint16_t checksum();

    void setSourcePort(uint16_t value);
    void setDestPort(uint16_t value);
    void setLength(uint16_t value);
    void setChecksum(uint16_t value);

    byte* data();
    uint16_t dataLen();
};

static_assert(sizeof(UdpHeader) == 8);

void udpRecv(NetworkInterface* netif, IpHeader* ipHeader, void* buffer, size_t size);
void udpBroadcast(NetworkInterface* netif, uint16_t sourcePort, uint16_t destPort,
                  void* buffer, size_t size);
void udpSend(IpAddress destIp, uint16_t sourcePort, uint16_t destPort, void* buffer,
             size_t size);
