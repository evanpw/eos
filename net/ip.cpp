#include "ip.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/bits.h"
#include "estd/new.h"
#include "estd/print.h"
#include "net/arp.h"
#include "net/ethernet.h"
#include "net/network_interface.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "spinlock.h"
#include "system.h"

IpAddress::IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    value = concatBits(d, c, b, a);
}

uint16_t IpHeader::computeChecksum() {
    uint32_t sum = 0;

    uint8_t* bytes = reinterpret_cast<uint8_t*>(this);
    for (size_t i = 0; i < headerLen() * 4; i += 2) {
        uint16_t word = (bytes[i] << 8) | bytes[i + 1];
        sum += word;
    }

    // Subtract off the embedded checksum
    sum -= ntohs(_checksum);

    // Include the overflow
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Ones complement
    return ~sum;
}

bool IpHeader::verifyChecksum() { return computeChecksum() == ntohs(_checksum); }
void IpHeader::fillChecksum() { _checksum = htons(computeChecksum()); }

uint8_t IpHeader::headerLen() { return _headerLen; }
uint8_t IpHeader::version() { return _version; }
uint8_t IpHeader::ecn() { return _ecn; }
uint8_t IpHeader::dscp() { return _dscp; }
uint16_t IpHeader::totalLen() { return ntohs(_totalLen); }
uint16_t IpHeader::identification() { return ntohs(_identification); }
uint16_t IpHeader::fragmentOffset() { return ntohs(_fragmentOffset); }
uint16_t IpHeader::flags() { return ntohs(_flags); }
uint8_t IpHeader::ttl() { return _ttl; }
IpProtocol IpHeader::protocol() { return (IpProtocol)_protocol; }
IpAddress IpHeader::sourceIp() { return IpAddress(_sourceIp); }
IpAddress IpHeader::destIp() { return IpAddress(_destIp); }

void IpHeader::setTotalLen(uint16_t value) { _totalLen = htons(value); }
void IpHeader::setProtocol(IpProtocol value) { _protocol = (uint8_t)value; }
void IpHeader::setDestIp(IpAddress value) { _destIp = value; }
void IpHeader::setSourceIp(IpAddress value) { _sourceIp = value; }

void ipRecv(NetworkInterface* netif, uint8_t* buffer, size_t size) {
    if (size < sizeof(IpHeader)) {
        return;
    }

    IpHeader* ipHeader = reinterpret_cast<IpHeader*>(buffer);
    if (ipHeader->version() != 4) return;
    if (ipHeader->headerLen() < 5) return;
    if (ipHeader->totalLen() < ipHeader->headerLen() * 4) return;
    if (ipHeader->totalLen() > size) return;
    if (!ipHeader->verifyChecksum()) return;
    if (ipHeader->destIp() != netif->ipAddress() &&
        ipHeader->destIp() != IpAddress::broadcast())
        return;

    buffer += ipHeader->headerLen() * 4;
    size = ipHeader->totalLen() - ipHeader->headerLen() * 4;

    switch (ipHeader->protocol()) {
        case IpProtocol::Udp:
            udpRecv(netif, ipHeader, buffer, size);
            break;

        case IpProtocol::Tcp:
            tcpRecv(ipHeader, buffer, size);
            break;

        default:
            break;
    }
}

struct Route {
    NetworkInterface* netif;
    MacAddress destMac;
};

struct PendingSend {
    uint8_t* packet;
    size_t totalSize;
    IpAddress nextHop;
    PendingSend* next = nullptr;
};

static Spinlock* ipLock = nullptr;
static PendingSend* sendQueue = nullptr;

void ipInit() {
    ipLock = new Spinlock();
    sendQueue = nullptr;
}

void queueSend(uint8_t* packet, size_t totalSize, IpAddress destIp) {
    ASSERT(ipLock->isLocked());
    PendingSend* pendingSend = new PendingSend{packet, totalSize, destIp};

    pendingSend->next = sendQueue;
    sendQueue = pendingSend;
}

estd::optional<IpAddress> getNextHop(IpAddress destIp) {
    ASSERT(ipLock->isLocked());

    // TODO: determine the correct network interface from the destination
    NetworkInterface* netif = &sys.netif();

    if (!netif->isConfigured()) {
        return {};
    }

    if (destIp == IpAddress::broadcast()) {
        return destIp;
    }

    return netif->isLocal(destIp) ? destIp : netif->gateway();
}

estd::optional<Route> findRoute(IpAddress destIp) {
    ASSERT(ipLock->isLocked());

    // TODO: determine the correct network interface from the destination
    NetworkInterface* netif = &sys.netif();

    if (!netif->isConfigured()) {
        return {};
    }

    if (destIp == IpAddress::broadcast()) {
        return {{netif, MacAddress::broadcast()}};
    }

    IpAddress nextHop = netif->isLocal(destIp) ? destIp : netif->gateway();
    if (auto result = arpLookup(netif, nextHop)) {
        return {{netif, *result}};
    }

    return {};
}

void ipRouteFound(IpAddress ip) {
    SpinlockLocker lock(*ipLock);

    auto route = findRoute(ip);
    if (!route) return;

    PendingSend** prevNext = &sendQueue;
    PendingSend* current = sendQueue;
    while (current) {
        if (current->nextHop == ip) {
            IpHeader* ipHeader = reinterpret_cast<IpHeader*>(current->packet);
            ipHeader->setSourceIp(route->netif->ipAddress());
            ipHeader->fillChecksum();

            ethSend(route->netif, route->destMac, EtherType::Ipv4, current->packet,
                    current->totalSize);

            // Delete the pending send and remove it from the list
            delete[] current->packet;
            *prevNext = current->next;
        }

        prevNext = &current->next;
        current = current->next;
    }
}

estd::optional<IpAddress> findRouteSourceIp(IpAddress) {
    // TODO: determine the correct network interface from the destination
    NetworkInterface* netif = &sys.netif();

    if (!netif->isConfigured()) {
        return {};
    }

    return {netif->ipAddress()};
}

bool ipSend(IpAddress destIp, IpProtocol protocol, void* buffer, size_t size) {
    size_t totalSize = sizeof(IpHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    IpHeader* ipHeader = new (packet) IpHeader;
    ipHeader->setTotalLen(totalSize);
    ipHeader->setProtocol(protocol);
    ipHeader->setDestIp(destIp);
    memcpy(packet + sizeof(IpHeader), buffer, size);

    // This is so we don't have a race condition between findRoute returning no value
    // and the send being queued. If the ARP reply happens between those two events, then
    // we'll never get the notification.
    SpinlockLocker lock(*ipLock);

    if (auto route = findRoute(destIp)) {
        ipHeader->setSourceIp(route->netif->ipAddress());
        ipHeader->fillChecksum();

        ethSend(route->netif, route->destMac, EtherType::Ipv4, packet, totalSize);
        delete[] packet;
    } else {
        auto nextHop = getNextHop(destIp);
        if (!nextHop) return false;

        queueSend(packet, totalSize, *nextHop);
    }

    return true;
}

bool ipBroadcast(NetworkInterface* netif, IpProtocol protocol, void* buffer,
                 size_t size) {
    size_t totalSize = sizeof(IpHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    IpHeader* ipHeader = new (packet) IpHeader;
    ipHeader->setTotalLen(totalSize);
    ipHeader->setProtocol(protocol);
    ipHeader->setSourceIp(netif->ipAddress());
    ipHeader->setDestIp(IpAddress::broadcast());
    ipHeader->fillChecksum();

    memcpy(packet + sizeof(IpHeader), buffer, size);
    ethSend(netif, MacAddress::broadcast(), EtherType::Ipv4, packet, totalSize);

    delete[] packet;
    return true;
}
