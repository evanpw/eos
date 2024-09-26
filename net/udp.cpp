#include "net/udp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/print.h"
#include "net/nic_device.h"

uint16_t UdpHeader::sourcePort() { return ntohs(_sourcePort); }

uint16_t UdpHeader::destPort() { return ntohs(_destPort); }

uint16_t UdpHeader::length() { return ntohs(_length); }

uint16_t UdpHeader::checksum() { return ntohs(_checksum); }

uint8_t* UdpHeader::data() { return _data; }

void udpRecv(NicDevice*, uint8_t* buffer, size_t size) {
    if (size < sizeof(UdpHeader)) {
        return;
    }

    UdpHeader* udpHeader = reinterpret_cast<UdpHeader*>(buffer);
    if (udpHeader->length() > size) return;

    // Echo the message to the console
    size_t dataLen = udpHeader->length() - sizeof(UdpHeader);
    char* s = new char[dataLen + 1];
    memcpy(s, udpHeader->data(), dataLen);
    s[dataLen] = '\0';
    println("udp net message: {}", s);
}
