#pragma once
#include <stddef.h>
#include <stdint.h>

struct IpAddress;
struct IpHeader;
class NicDevice;

class __attribute__((packed)) TcpHeader {
    uint16_t _sourcePort;
    uint16_t _destPort;
    uint32_t _seqNum;
    uint32_t _ackNum = 0;
    [[maybe_unused]] uint8_t _reserved : 4 = 0;
    uint8_t _dataOffset : 4 = 5;
    uint8_t _fin : 1 = 0;
    uint8_t _syn : 1 = 0;
    uint8_t _rst : 1 = 0;
    uint8_t _psh : 1 = 0;
    uint8_t _ack : 1 = 0;
    uint8_t _urg : 1 = 0;
    uint8_t _ece : 1 = 0;
    uint8_t _cwr : 1 = 0;
    uint16_t _windowSize;
    uint16_t _checksum;
    uint16_t _urgentPointer = 0;
    uint8_t _data[];

    uint16_t computeChecksum(IpAddress srcIp, IpAddress destIp, size_t totalLen);

public:
    bool verifyChecksum(IpHeader* ipHeader);
    void fillChecksum(IpAddress srcIp, IpAddress destIp, size_t totalLen);

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

    void setSourcePort(uint16_t value);
    void setDestPort(uint16_t value);
    void setSeqNum(uint32_t value);
    void setAckNum(uint32_t value);
    void setDataOffset(uint8_t value);
    void setFin();
    void setSyn();
    void setRst();
    void setPsh();
    void setAck();
    void setUrg();
    void setEce();
    void setCwr();
    void setWindowSize(uint16_t value);
    void setChecksum(uint16_t value);
    void setUrgentPointer(uint16_t value);
};

static_assert(sizeof(TcpHeader) == 20);

void tcpInit();
void tcpRecv(NicDevice* nic, IpHeader* ipHeader, uint8_t* buffer, size_t size);

struct TcpControlBlock;
TcpControlBlock* tcpConnect(NicDevice* nic, IpAddress destIp, uint16_t destPort);

void tcpSend(NicDevice* nic, TcpControlBlock* tcb, const uint8_t* data, size_t size);
void tcpClose(NicDevice* nic, TcpControlBlock* tcb);
