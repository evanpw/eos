#include "ip.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/bits.h"
#include "estd/new.h"
#include "estd/print.h"
#include "net/arp.h"
#include "net/ethernet.h"
#include "net/nic_device.h"
#include "net/tcp.h"
#include "net/udp.h"

IpAddress::IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    value = concatBits(d, c, b, a);
}

void IpAddress::print() {
    uint8_t bytes[4];
    memcpy(bytes, &value, sizeof(uint32_t));

    ::print("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
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

void ipRecv(NicDevice* nic, uint8_t* buffer, size_t size) {
    if (size < sizeof(IpHeader)) {
        println("net: malformed ipv4 packet: too short");
        return;
    }

    IpHeader* ipHeader = reinterpret_cast<IpHeader*>(buffer);
    if (ipHeader->version() != 4) return;
    if (ipHeader->headerLen() < 5) return;
    if (ipHeader->totalLen() < ipHeader->headerLen() * 4) return;
    if (ipHeader->totalLen() > size) return;
    if (!ipHeader->verifyChecksum()) return;
    if (ipHeader->destIp() != nic->ipAddress()) return;

    buffer += ipHeader->headerLen() * 4;
    size = ipHeader->totalLen() - ipHeader->headerLen() * 4;

    switch (ipHeader->protocol()) {
        case IpProtocol::Udp:
            udpRecv(nic, ipHeader, buffer, size);
            break;

        case IpProtocol::Tcp:
            tcpRecv(nic, ipHeader, buffer, size);
            break;

        default:
            break;
    }
}

void ipSend(NicDevice* nic, IpAddress destIp, IpProtocol protocol, void* buffer,
            size_t size) {
    MacAddress destMac;
    if (!arpLookup(destIp, &destMac)) {
        print("net: arp lookup failed for ip ");
        destIp.print();
        println("");
        return;
    }

    size_t totalSize = sizeof(IpHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    IpHeader* ipHeader = new (packet) IpHeader;
    ipHeader->setTotalLen(totalSize);
    ipHeader->setProtocol(protocol);
    ipHeader->setSourceIp(nic->ipAddress());
    ipHeader->setDestIp(destIp);
    ipHeader->fillChecksum();

    // TODO: fill in UDP or TCP checksum

    memcpy(packet + sizeof(IpHeader), buffer, size);
    ethSend(nic, destMac, EtherType::Ipv4, packet, totalSize);
}
