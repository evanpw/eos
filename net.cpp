#include "net.h"

#include <arpa/inet.h>
#include <string.h>

#include "e1000.h"
#include "estd/print.h"
#include "system.h"

void printMacAddress(MacAddress mac) {
    print("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
}

void printIpAddress(IpAddress ip) { print("{}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]); }

enum class EtherType : uint16_t {
    Ipv4 = 0x0800,
    Arp = 0x0806,
    Ipv6 = 0x86DD,
};

class __attribute__((packed)) EthernetHeader {
    uint8_t _destMac[6];
    uint8_t _srcMac[6];
    uint16_t _etherType;

public:
    MacAddress destMac() {
        MacAddress result;
        memcpy(&result, &_destMac, 6);
        return result;
    }

    void setDestMac(MacAddress value) { memcpy(&_destMac, &value, 6); }

    MacAddress srcMac() {
        MacAddress result;
        memcpy(&result, &_srcMac, 6);
        return result;
    }

    void setSrcMac(MacAddress value) { memcpy(&_srcMac, &value, 6); }

    EtherType etherType() { return (EtherType)ntohs(_etherType); }
    void setEtherType(EtherType value) { _etherType = htons((uint16_t)value); }
};

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
    ArpHardwareType hardwareType() { return (ArpHardwareType)ntohs(_hardwareType); }

    void setHardwareType(ArpHardwareType value) {
        _hardwareType = htons((uint16_t)value);
    }

    EtherType protocolType() { return (EtherType)ntohs(_protocolType); }
    void setProtocolType(EtherType value) { _protocolType = htons((uint16_t)value); }

    uint8_t hardwareLen() { return _hardwareLen; }
    void setHardwareLen(uint8_t value) { _hardwareLen = value; }

    uint8_t protocolLen() { return _protocolLen; }
    void setProtocolLen(uint8_t value) { _protocolLen = value; }

    ArpOperation operation() { return (ArpOperation)ntohs(_operation); }
    void setOperation(ArpOperation value) { _operation = htons((uint16_t)value); }

    MacAddress senderMac() {
        MacAddress result;
        memcpy(&result, &_senderMac, 6);
        return result;
    }

    void setSenderMac(MacAddress value) { memcpy(&_senderMac, &value, 6); }

    IpAddress senderIp() {
        IpAddress result;
        memcpy(&result, &_senderIp, 4);
        return result;
    }

    void setSenderIp(IpAddress value) { memcpy(&_senderIp, &value, 4); }

    MacAddress targetMac() {
        MacAddress result;
        memcpy(&result, &_targetMac, 6);
        return result;
    }

    void setTargetMac(MacAddress value) { memcpy(&_targetMac, &value, 6); }

    IpAddress targetIp() {
        IpAddress result;
        memcpy(&result, &_targetIp, 4);
        return result;
    }

    void setTargetIp(IpAddress value) { memcpy(&_targetIp, &value, 4); }
};

void handleArp(E1000Device* nic, EthernetHeader* ethHeader, uint8_t* buffer,
               size_t size) {
    if (size < sizeof(ArpHeader)) {
        println("net: malformed arp packet: too short");
        return;
    }

    // Look for ethernet->ipv4 arp request packets
    ArpHeader* arpPacket = reinterpret_cast<ArpHeader*>(buffer);

    // Look for ethernet->ipv4 arp request packets
    if (arpPacket->hardwareType() != ArpHardwareType::Ethernet) return;
    if (arpPacket->protocolType() != EtherType::Ipv4) return;
    if (arpPacket->hardwareLen() != 6) return;
    if (arpPacket->protocolLen() != 4) return;
    if (arpPacket->operation() != ArpOperation::Request) return;

    // Construct an ARP reply packet
    PhysicalAddress packetBase = System::mm().pageAlloc(1);
    uint8_t* packetAddr = System::mm().physicalToVirtual(packetBase).ptr<uint8_t>();

    EthernetHeader* replyEthHeader = reinterpret_cast<EthernetHeader*>(packetAddr);
    replyEthHeader->setDestMac(ethHeader->srcMac());
    replyEthHeader->setSrcMac(nic->macAddress());
    replyEthHeader->setEtherType(EtherType::Arp);

    ArpHeader* replyArpHeader =
        reinterpret_cast<ArpHeader*>(packetAddr + sizeof(EthernetHeader));
    replyArpHeader->setHardwareType(ArpHardwareType::Ethernet);
    replyArpHeader->setProtocolType(EtherType::Ipv4);
    replyArpHeader->setHardwareLen(6);
    replyArpHeader->setProtocolLen(4);
    replyArpHeader->setOperation(ArpOperation::Reply);
    replyArpHeader->setSenderMac(nic->macAddress());
    replyArpHeader->setSenderIp(arpPacket->targetIp());
    replyArpHeader->setTargetMac(arpPacket->senderMac());
    replyArpHeader->setTargetIp(arpPacket->senderIp());

    // Send it (no payload)
    nic->sendPacket(packetBase, sizeof(EthernetHeader) + sizeof(ArpHeader));
    System::mm().pageFree(packetBase);
}

class __attribute__((packed)) IpHeader {
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
    uint8_t headerLen() { return _headerLen; }
    uint8_t version() { return _version; }
    uint8_t ecn() { return _ecn; }
    uint8_t dscp() { return _dscp; }
    uint16_t totalLen() { return ntohs(_totalLen); }
    uint16_t identification() { return ntohs(_identification); }
    uint16_t fragmentOffset() { return ntohs(_fragmentOffset); }
    uint16_t flags() { return ntohs(_flags); }
    uint8_t ttl() { return _ttl; }
    uint8_t protocol() { return _protocol; }
    uint16_t checksum() { return ntohs(_checksum); }

    IpAddress sourceIp() {
        IpAddress result;
        memcpy(&result, &_sourceIp, 4);
        return result;
    }

    IpAddress destIp() {
        IpAddress result;
        memcpy(&result, &_destIp, 4);
        return result;
    }
};
static_assert(sizeof(IpHeader) == 20);

class __attribute__((packed)) UdpHeader {
    uint16_t _sourcePort;
    uint16_t _destPort;
    uint16_t _length;
    uint16_t _checksum;
    uint8_t _data[];

public:
    uint16_t sourcePort() { return ntohs(_sourcePort); }
    uint16_t destPort() { return ntohs(_destPort); }
    uint16_t length() { return ntohs(_length); }
    uint16_t checksum() { return ntohs(_checksum); }
    uint8_t* data() { return _data; }
};

void handleUdp(uint8_t* buffer, size_t size) {
    if (size < sizeof(UdpHeader)) {
        println("net: malformed udp packet: too short");
        return;
    }

    UdpHeader* udpHeader = reinterpret_cast<UdpHeader*>(buffer);
    size_t dataLen = udpHeader->length() - sizeof(UdpHeader);

    char* s = new char[dataLen + 1];
    memcpy(s, udpHeader->data(), dataLen);
    s[dataLen] = '\0';

    println("net message: {}", s);
}

void handleIp(uint8_t* buffer, size_t size) {
    if (size < sizeof(IpHeader)) {
        println("net: malformed ipv4 packet: too short");
        return;
    }

    IpHeader* ipHeader = reinterpret_cast<IpHeader*>(buffer);
    buffer += ipHeader->headerLen() * 4;
    size = ipHeader->totalLen() - ipHeader->headerLen() * 4;

    switch (ipHeader->protocol()) {
        case 17:  // UDP
            handleUdp(buffer, size);
            break;

        default:
            break;
    }
}

void handleEthernet(E1000Device* nic, uint8_t* buffer, size_t size) {
    if (size < sizeof(EthernetHeader)) {
        println("net: malformed ethernet packet: too short");
        return;
    }

    EthernetHeader* ethHeader = reinterpret_cast<EthernetHeader*>(buffer);
    buffer += sizeof(EthernetHeader);
    size -= sizeof(EthernetHeader);

    switch (ethHeader->etherType()) {
        case EtherType::Arp:
            handleArp(nic, ethHeader, buffer, size);
            break;

        case EtherType::Ipv4:
            handleIp(buffer, size);
            break;

        default:
            break;
    }
}

void handlePacket(E1000Device* nic, uint8_t* buffer, size_t size) {
    handleEthernet(nic, buffer, size);
}
