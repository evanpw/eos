#pragma once
#include <stddef.h>
#include <stdint.h>

class NicDevice;

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
    uint8_t* data();
};

static_assert(sizeof(UdpHeader) == 8);

void udpRecv(NicDevice* nic, uint8_t* buffer, size_t size);
