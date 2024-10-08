#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/optional.h"

enum class EtherType : uint16_t;
class NetworkInterface;
struct MacAddress;
struct IpAddress;
class EthernetHeader;

enum class ArpHardwareType : uint16_t {
    // Values are in network byte order
    Ethernet = 0x0100,
};

enum class ArpOperation : uint16_t {
    // Values are in network byte order
    Request = 0x0100,
    Reply = 0x0200,
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
    EtherType protocolType();
    uint8_t hardwareLen();
    uint8_t protocolLen();
    ArpOperation operation();
    MacAddress senderMac();
    IpAddress senderIp();
    MacAddress targetMac();
    IpAddress targetIp();

    void setHardwareType(ArpHardwareType value);
    void setProtocolType(EtherType value);
    void setHardwareLen(uint8_t value);
    void setProtocolLen(uint8_t value);
    void setOperation(ArpOperation value);
    void setSenderMac(MacAddress value);
    void setSenderIp(IpAddress value);
    void setTargetMac(MacAddress value);
    void setTargetIp(IpAddress value);
};

static_assert(sizeof(ArpHeader) == 28);

void arpInit();

estd::optional<MacAddress> arpLookup(NetworkInterface* netif, IpAddress ip);

void arpRecv(NetworkInterface* netif, uint8_t* buffer, size_t size);
void arpRequest(NetworkInterface* netif, IpAddress destIp);
void arpReply(NetworkInterface* netif, MacAddress destMac, IpAddress destIp);
