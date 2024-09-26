#include "net/ethernet.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "estd/print.h"
#include "net/arp.h"
#include "net/ip.h"
#include "net/nic_device.h"

MacAddress MacAddress::broadcast() {
    MacAddress result;
    memset(result.bytes, 0xFF, sizeof(MacAddress));
    return result;
}

bool MacAddress::operator==(const MacAddress& other) const {
    return memcmp(bytes, other.bytes, sizeof(MacAddress)) == 0;
}

void MacAddress::print() {
    ::print("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", bytes[0], bytes[1], bytes[2],
            bytes[3], bytes[4], bytes[5]);
}

MacAddress EthernetHeader::destMac() {
    MacAddress result;
    memcpy(&result, &_destMac, sizeof(MacAddress));
    return result;
}

void EthernetHeader::setDestMac(MacAddress value) {
    memcpy(&_destMac, &value, sizeof(MacAddress));
}

MacAddress EthernetHeader::srcMac() {
    MacAddress result;
    memcpy(&result, &_srcMac, sizeof(MacAddress));
    return result;
}

void EthernetHeader::setSrcMac(MacAddress value) {
    memcpy(&_srcMac, &value, sizeof(MacAddress));
}

EtherType EthernetHeader::etherType() { return (EtherType)ntohs(_etherType); }

void EthernetHeader::setEtherType(EtherType value) {
    _etherType = htons((uint16_t)value);
}

void ethRecv(NicDevice* nic, uint8_t* buffer, size_t size) {
    if (size < sizeof(EthernetHeader)) {
        return;
    }

    EthernetHeader* ethHeader = reinterpret_cast<EthernetHeader*>(buffer);
    if ((ethHeader->destMac() != nic->macAddress()) &&
        (ethHeader->destMac() != MacAddress::broadcast())) {
        return;
    }

    buffer += sizeof(EthernetHeader);
    size -= sizeof(EthernetHeader);

    switch (ethHeader->etherType()) {
        case EtherType::Arp:
            arpRecv(nic, buffer, size);
            break;

        case EtherType::Ipv4:
            ipRecv(nic, buffer, size);
            break;

        default:
            break;
    }
}

void ethSend(NicDevice* nic, MacAddress destMac, EtherType ethType, void* buffer,
             size_t size) {
    size_t totalSize = sizeof(EthernetHeader) + size;
    uint8_t* packet = new uint8_t[totalSize];

    EthernetHeader* ethHeader = new (packet) EthernetHeader;
    ethHeader->setDestMac(destMac);
    ethHeader->setSrcMac(nic->macAddress());
    ethHeader->setEtherType(ethType);

    memcpy(packet + sizeof(EthernetHeader), buffer, size);
    nic->sendPacket(packet, totalSize);
}
