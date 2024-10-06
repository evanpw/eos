#include "net/udp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "estd/print.h"
#include "net/dhcp.h"
#include "net/dns.h"
#include "net/ip.h"
#include "net/network_interface.h"

uint16_t UdpHeader::sourcePort() { return ntohs(_sourcePort); }
uint16_t UdpHeader::destPort() { return ntohs(_destPort); }
uint16_t UdpHeader::length() { return ntohs(_length); }
uint16_t UdpHeader::checksum() { return ntohs(_checksum); }
uint8_t* UdpHeader::data() { return _data; }
uint16_t UdpHeader::dataLen() { return length() - sizeof(UdpHeader); }

void UdpHeader::setSourcePort(uint16_t value) { _sourcePort = htons(value); }
void UdpHeader::setDestPort(uint16_t value) { _destPort = htons(value); }
void UdpHeader::setLength(uint16_t value) { _length = htons(value); }
void UdpHeader::setChecksum(uint16_t value) { _checksum = htons(value); }

void udpRecv(NetworkInterface* netif, IpHeader* ipHeader, uint8_t* buffer, size_t size) {
    if (size < sizeof(UdpHeader)) {
        return;
    }

    UdpHeader* udpHeader = reinterpret_cast<UdpHeader*>(buffer);
    if (udpHeader->length() > size) return;

    if (udpHeader->destPort() == 22) {
        // Echo the message to the console
        size_t dataLen = udpHeader->length() - sizeof(UdpHeader);
        char* s = new char[dataLen + 1];
        memcpy(s, udpHeader->data(), dataLen);
        s[dataLen] = '\0';
        println("udp net message: {}", s);

        // Echo the message back
        udpSend(ipHeader->sourceIp(), 22, udpHeader->sourcePort(), udpHeader->data(),
                dataLen);
    } else if (udpHeader->destPort() == DHCP_CLIENT_PORT) {
        dhcpRecv(netif, ipHeader, udpHeader->data(), udpHeader->dataLen());
    } else if (udpHeader->destPort() == 10053) {
        dnsRecv(udpHeader->data(), udpHeader->dataLen());
    }
}

void udpSend(IpAddress destIp, uint16_t sourcePort, uint16_t destPort, uint8_t* buffer,
             uint8_t size, bool blocking) {
    size_t totalSize = sizeof(UdpHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    UdpHeader* udpHeader = new (packet) UdpHeader;
    udpHeader->setSourcePort(sourcePort);
    udpHeader->setDestPort(destPort);
    udpHeader->setLength(totalSize);
    udpHeader->setChecksum(0);

    memcpy(udpHeader->data(), buffer, size);
    ipSend(destIp, IpProtocol::Udp, packet, totalSize, blocking);

    delete[] packet;
}

void udpBroadcast(NetworkInterface* netif, uint16_t sourcePort, uint16_t destPort,
                  uint8_t* buffer, uint8_t size) {
    size_t totalSize = sizeof(UdpHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    UdpHeader* udpHeader = new (packet) UdpHeader;
    udpHeader->setSourcePort(sourcePort);
    udpHeader->setDestPort(destPort);
    udpHeader->setLength(totalSize);
    udpHeader->setChecksum(0);

    memcpy(udpHeader->data(), buffer, size);
    ipBroadcast(netif, IpProtocol::Udp, packet, totalSize);

    delete[] packet;
}
