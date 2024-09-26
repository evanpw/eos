#include "net/arp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/nic_device.h"
#include "spinlock.h"

ArpHardwareType ArpHeader::hardwareType() {
    return (ArpHardwareType)ntohs(_hardwareType);
}

void ArpHeader::setHardwareType(ArpHardwareType value) {
    _hardwareType = htons((uint16_t)value);
}

EtherType ArpHeader::protocolType() { return (EtherType)ntohs(_protocolType); }

void ArpHeader::setProtocolType(EtherType value) {
    _protocolType = htons((uint16_t)value);
}

uint8_t ArpHeader::hardwareLen() { return _hardwareLen; }

void ArpHeader::setHardwareLen(uint8_t value) { _hardwareLen = value; }

uint8_t ArpHeader::protocolLen() { return _protocolLen; }

void ArpHeader::setProtocolLen(uint8_t value) { _protocolLen = value; }

ArpOperation ArpHeader::operation() { return (ArpOperation)ntohs(_operation); }

void ArpHeader::setOperation(ArpOperation value) { _operation = htons((uint16_t)value); }

MacAddress ArpHeader::senderMac() {
    MacAddress result;
    memcpy(&result, &_senderMac, sizeof(MacAddress));
    return result;
}

void ArpHeader::setSenderMac(MacAddress value) {
    memcpy(&_senderMac, &value, sizeof(MacAddress));
}

IpAddress ArpHeader::senderIp() {
    IpAddress result;
    memcpy(&result, &_senderIp, sizeof(IpAddress));
    return result;
}

void ArpHeader::setSenderIp(IpAddress value) {
    memcpy(&_senderIp, &value, sizeof(IpAddress));
}

MacAddress ArpHeader::targetMac() {
    MacAddress result;
    memcpy(&result, &_targetMac, sizeof(MacAddress));
    return result;
}

void ArpHeader::setTargetMac(MacAddress value) {
    memcpy(&_targetMac, &value, sizeof(MacAddress));
}

IpAddress ArpHeader::targetIp() {
    IpAddress result;
    memcpy(&result, &_targetIp, sizeof(IpAddress));
    return result;
}

void ArpHeader::setTargetIp(IpAddress value) {
    memcpy(&_targetIp, &value, sizeof(IpAddress));
}

struct ArpEntry {
    IpAddress ip;
    MacAddress mac;
    ArpEntry* next;
};

static ArpEntry* arpCache = nullptr;
static Spinlock* arpLock = nullptr;

void arpInit() { arpLock = new Spinlock(); }

bool arpLookup(IpAddress ip, MacAddress* result) {
    SpinlockLocker locker(*arpLock);

    ArpEntry* entry = arpCache;
    while (entry != nullptr) {
        if (entry->ip == ip) {
            *result = entry->mac;
            return true;
        }

        entry = entry->next;
    }

    return false;
}

static void arpInsert(IpAddress ip, MacAddress mac) {
    SpinlockLocker locker(*arpLock);

    // If we already have an entry for this ip, just update the mac
    ArpEntry* entry = arpCache;
    while (entry != nullptr) {
        if (entry->ip == ip) {
            entry->mac = mac;
            return;
        }

        entry = entry->next;
    }

    // Otherwise, add it to the list
    ArpEntry* newEntry = new ArpEntry;
    newEntry->ip = ip;
    newEntry->mac = mac;
    newEntry->next = arpCache;
    arpCache = newEntry;
}

void arpRecv(NicDevice* nic, uint8_t* buffer, size_t size) {
    if (size < sizeof(ArpHeader)) {
        return;
    }

    ArpHeader* arpPacket = reinterpret_cast<ArpHeader*>(buffer);

    // Look for ethernet->ipv4 arp request packets
    if (arpPacket->hardwareType() != ArpHardwareType::Ethernet) return;
    if (arpPacket->protocolType() != EtherType::Ipv4) return;
    if (arpPacket->hardwareLen() != sizeof(MacAddress)) return;
    if (arpPacket->protocolLen() != sizeof(IpAddress)) return;
    if (arpPacket->operation() != ArpOperation::Request) return;
    if (arpPacket->targetIp() != nic->ipAddress()) return;

    arpInsert(arpPacket->senderIp(), arpPacket->senderMac());
    arpReply(nic, arpPacket->senderMac(), arpPacket->senderIp());
}

void arpReply(NicDevice* nic, MacAddress destMac, IpAddress destIp) {
    ArpHeader arpHeader;
    arpHeader.setHardwareType(ArpHardwareType::Ethernet);
    arpHeader.setProtocolType(EtherType::Ipv4);
    arpHeader.setHardwareLen(sizeof(MacAddress));
    arpHeader.setProtocolLen(sizeof(IpAddress));
    arpHeader.setOperation(ArpOperation::Reply);
    arpHeader.setSenderMac(nic->macAddress());
    arpHeader.setSenderIp(nic->ipAddress());
    arpHeader.setTargetMac(destMac);
    arpHeader.setTargetIp(destIp);

    ethSend(nic, destMac, EtherType::Arp, &arpHeader, sizeof(ArpHeader));
}
