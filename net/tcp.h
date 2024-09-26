#pragma once
#include <stddef.h>
#include <stdint.h>

class NicDevice;

class __attribute__((packed)) TcpHeader {
    uint16_t _sourcePort;
    uint16_t _destPort;
    uint32_t _seqNum;
    uint32_t _ackNum;
    [[maybe_unused]] uint8_t _reserved : 4;
    uint8_t _dataOffset : 4;
    uint8_t _fin : 1;
    uint8_t _syn : 1;
    uint8_t _rst : 1;
    uint8_t _psh : 1;
    uint8_t _ack : 1;
    uint8_t _urg : 1;
    uint8_t _ece : 1;
    uint8_t _cwr : 1;
    uint16_t _windowSize;
    uint16_t _checksum;
    uint16_t _urgentPointer;
    uint8_t _data[];

public:
    uint16_t sourcePort();
    uint16_t destPort();
    uint32_t seqNum();
    uint32_t ackNum();
    uint8_t dataOffset();
    uint8_t fin();
    uint8_t syn();
    uint8_t rst();
    uint8_t psh();
    uint8_t ack();
    uint8_t urg();
    uint8_t ece();
    uint8_t cwr();
    uint16_t windowSize();
    uint16_t checksum();
    uint16_t urgentPointer();
    uint8_t* data();
};

static_assert(sizeof(TcpHeader) == 20);

void tcpRecv(NicDevice* nic, uint8_t* buffer, size_t size);
