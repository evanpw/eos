#include "ip.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/print.h"
#include "net/nic_device.h"
#include "net/tcp.h"
#include "net/udp.h"

bool IpAddress::operator==(const IpAddress& other) const {
    return memcmp(bytes, other.bytes, sizeof(IpAddress)) == 0;
}

void IpAddress::print() {
    ::print("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
}

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

uint16_t IpHeader::checksum() { return ntohs(_checksum); }

IpAddress IpHeader::sourceIp() {
    IpAddress result;
    memcpy(&result, &_sourceIp, 4);
    return result;
}

IpAddress IpHeader::destIp() {
    IpAddress result;
    memcpy(&result, &_destIp, 4);
    return result;
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
    if (ipHeader->computeChecksum() != ipHeader->checksum()) return;
    if (ipHeader->destIp() != nic->ipAddress()) return;

    buffer += ipHeader->headerLen() * 4;
    size = ipHeader->totalLen() - ipHeader->headerLen() * 4;

    switch (ipHeader->protocol()) {
        case IpProtocol::Udp:
            udpRecv(nic, buffer, size);
            break;

        case IpProtocol::Tcp:
            tcpRecv(nic, buffer, size);
            break;

        default:
            break;
    }
}
