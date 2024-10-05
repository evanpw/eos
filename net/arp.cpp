#include "net/arp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "estd/vector.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/network_interface.h"
#include "scheduler.h"
#include "spinlock.h"
#include "system.h"

EtherType ArpHeader::protocolType() { return (EtherType)_protocolType; }
uint8_t ArpHeader::hardwareLen() { return _hardwareLen; }
uint8_t ArpHeader::protocolLen() { return _protocolLen; }
ArpOperation ArpHeader::operation() { return (ArpOperation)_operation; }
MacAddress ArpHeader::senderMac() { return MacAddress(_senderMac); }
MacAddress ArpHeader::targetMac() { return MacAddress(_targetMac); }
IpAddress ArpHeader::senderIp() { return IpAddress(_senderIp); }
IpAddress ArpHeader::targetIp() { return IpAddress(_targetIp); }
ArpHardwareType ArpHeader::hardwareType() { return (ArpHardwareType)_hardwareType; }

void ArpHeader::setHardwareLen(uint8_t value) { _hardwareLen = value; }
void ArpHeader::setProtocolLen(uint8_t value) { _protocolLen = value; }
void ArpHeader::setOperation(ArpOperation value) { _operation = (uint16_t)value; }
void ArpHeader::setSenderIp(IpAddress value) { _senderIp = value; }
void ArpHeader::setTargetIp(IpAddress value) { _targetIp = value; }
void ArpHeader::setProtocolType(EtherType value) { _protocolType = (uint16_t)value; }

void ArpHeader::setHardwareType(ArpHardwareType value) {
    _hardwareType = (uint16_t)value;
}

void ArpHeader::setSenderMac(MacAddress value) {
    memcpy(&_senderMac, &value, sizeof(MacAddress));
}

void ArpHeader::setTargetMac(MacAddress value) {
    memcpy(&_targetMac, &value, sizeof(MacAddress));
}

struct ArpEntry {
    IpAddress ip;
    MacAddress mac;
    ArpEntry* next;
};

struct ArpBlocker : Blocker {
    ArpBlocker(IpAddress ipAddress) : ipAddress(ipAddress) {}
    IpAddress ipAddress;
};

static ArpEntry* arpCache = nullptr;
static Spinlock* arpLock = nullptr;
estd::vector<estd::shared_ptr<ArpBlocker>> arpBlockers;

static void arpInsert(IpAddress ip, MacAddress mac);

void arpInit() {
    arpLock = new Spinlock();
    new (&arpBlockers) decltype(arpBlockers);
}

estd::shared_ptr<ArpBlocker> getBlocker(IpAddress ip, bool create = false) {
    for (auto& blocker : arpBlockers) {
        if (blocker->ipAddress == ip) {
            return blocker;
        }
    }

    if (create) {
        estd::shared_ptr<ArpBlocker> blocker(new ArpBlocker(ip));
        arpBlockers.push_back(blocker);
        return blocker;
    }

    return {};
}

bool arpLookupCachedLocked(IpAddress ip, MacAddress* result) {
    ASSERT(arpLock->isLocked());

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

bool arpLookupCached(IpAddress ip, MacAddress* result) {
    SpinlockLocker locker(*arpLock);
    return arpLookupCachedLocked(ip, result);
}

MacAddress arpLookup(NetworkInterface* netif, IpAddress ip) {
    MacAddress result;
    if (arpLookupCached(ip, &result)) return result;

    // Not in cache, we have to send an arp request
    arpRequest(netif, ip);

    // Wait for the reply. We need to take the lock to avoid a race condition between
    // checking the cache and going to sleep. Otherwise, the reply may arrive between
    // those two operations and we could miss it and sleep forever.
    while (true) {
        SpinlockLocker locker(*arpLock);
        if (arpLookupCachedLocked(ip, &result)) {
            return result;
        }

        sys.scheduler().sleepThread(getBlocker(ip, true), arpLock);
    }
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

    // Wake up any threads waiting for this ip
    auto blocker = getBlocker(ip);
    if (blocker) {
        sys.scheduler().wakeThreads(blocker);
    }
}

void arpRecv(NetworkInterface* netif, uint8_t* buffer, size_t size) {
    if (size < sizeof(ArpHeader)) {
        return;
    }

    ArpHeader* arpPacket = reinterpret_cast<ArpHeader*>(buffer);

    // Look for ethernet->ipv4 arp request or reply packets
    if (arpPacket->hardwareType() != ArpHardwareType::Ethernet) return;
    if (arpPacket->protocolType() != EtherType::Ipv4) return;
    if (arpPacket->hardwareLen() != sizeof(MacAddress)) return;
    if (arpPacket->protocolLen() != sizeof(IpAddress)) return;
    if (arpPacket->targetIp() != netif->ipAddress()) return;

    arpInsert(arpPacket->senderIp(), arpPacket->senderMac());

    if (arpPacket->operation() == ArpOperation::Request) {
        arpReply(netif, arpPacket->senderMac(), arpPacket->senderIp());
    }
}

void arpRequest(NetworkInterface* netif, IpAddress destIp) {
    ArpHeader arpHeader;
    arpHeader.setHardwareType(ArpHardwareType::Ethernet);
    arpHeader.setProtocolType(EtherType::Ipv4);
    arpHeader.setHardwareLen(sizeof(MacAddress));
    arpHeader.setProtocolLen(sizeof(IpAddress));
    arpHeader.setOperation(ArpOperation::Request);
    arpHeader.setSenderMac(netif->macAddress());
    arpHeader.setSenderIp(netif->ipAddress());
    arpHeader.setTargetMac(MacAddress());
    arpHeader.setTargetIp(destIp);

    ethSend(netif, MacAddress::broadcast(), EtherType::Arp, &arpHeader,
            sizeof(ArpHeader));
}

void arpReply(NetworkInterface* netif, MacAddress destMac, IpAddress destIp) {
    ArpHeader arpHeader;
    arpHeader.setHardwareType(ArpHardwareType::Ethernet);
    arpHeader.setProtocolType(EtherType::Ipv4);
    arpHeader.setHardwareLen(sizeof(MacAddress));
    arpHeader.setProtocolLen(sizeof(IpAddress));
    arpHeader.setOperation(ArpOperation::Reply);
    arpHeader.setSenderMac(netif->macAddress());
    arpHeader.setSenderIp(netif->ipAddress());
    arpHeader.setTargetMac(destMac);
    arpHeader.setTargetIp(destIp);

    ethSend(netif, destMac, EtherType::Arp, &arpHeader, sizeof(ArpHeader));
}
