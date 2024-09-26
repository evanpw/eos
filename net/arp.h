#pragma once
#include <stddef.h>
#include <stdint.h>

enum class EtherType : uint16_t;
class NicDevice;
struct MacAddress;
struct IpAddress;
class EthernetHeader;

enum class ArpHardwareType : uint16_t {
    Ethernet = 1,
};

enum class ArpOperation : uint16_t {
    Request = 1,
    Reply = 2,
};

class __attribute__((packed)) ArpHeader {
    uint16_t _hardwareType;
    uint16_t _protocolType;
    uint8_t _hardwareLen;
    uint8_t _protocolLen;
    uint16_t _operation;
    uint8_t _senderMac[6];
    uint32_t _senderIp;
    uint8_t _targetMac[6];
    uint32_t _targetIp;

public:
    ArpHardwareType hardwareType();
    void setHardwareType(ArpHardwareType value);

    EtherType protocolType();
    void setProtocolType(EtherType value);

    uint8_t hardwareLen();
    void setHardwareLen(uint8_t value);

    uint8_t protocolLen();
    void setProtocolLen(uint8_t value);

    ArpOperation operation();
    void setOperation(ArpOperation value);

    MacAddress senderMac();
    void setSenderMac(MacAddress value);

    IpAddress senderIp();
    void setSenderIp(IpAddress value);

    MacAddress targetMac();
    void setTargetMac(MacAddress value);

    IpAddress targetIp();
    void setTargetIp(IpAddress value);
};

static_assert(sizeof(ArpHeader) == 28);

void arpRecv(NicDevice* nic, uint8_t* buffer, size_t size);
void arpReply(NicDevice* nic, MacAddress destMac, IpAddress destIp);
