#include "net/dns.h"

#include <arpa/inet.h>

#include "net/udp.h"

uint16_t DnsHeader::id() { return ntohs(_id); }
bool DnsHeader::isReply() { return _qr; }
DnsOpcode DnsHeader::opcode() { return (DnsOpcode)_opcode; }
bool DnsHeader::authoritativeAnswer() { return _aa; }
bool DnsHeader::truncated() { return _tc; }
bool DnsHeader::recursionDesired() { return _rd; }
bool DnsHeader::recursionAvailable() { return _ra; }
DnsResponseCode DnsHeader::rcode() { return (DnsResponseCode)_rcode; }
uint16_t DnsHeader::flags() { return ntohs(_flags); }
uint16_t DnsHeader::qdcount() { return ntohs(_qdcount); }
uint16_t DnsHeader::ancount() { return ntohs(_ancount); }
uint16_t DnsHeader::nscount() { return ntohs(_nscount); }
uint16_t DnsHeader::arcount() { return ntohs(_arcount); }

void DnsHeader::setId(uint16_t value) { _id = htons(value); }
void DnsHeader::setOpcode(DnsOpcode value) { _opcode = (uint16_t)value; }
void DnsHeader::setRecursionDesired() { _rd = 1; }
void DnsHeader::setQdcount(uint16_t value) { _qdcount = htons(value); }
void DnsHeader::setAncount(uint16_t value) { _ancount = htons(value); }
void DnsHeader::setNscount(uint16_t value) { _nscount = htons(value); }
void DnsHeader::setArcount(uint16_t value) { _arcount = htons(value); }

IpAddress dnsResolve(NetworkInterface* netif, IpAddress dnsServer, const char* hostname) {
    // clang-format off
    const char questions[19] = {
        2, 'g', 'h',
        6, 'e', 'v', 'a', 'n', 'p', 'w',
        3, 'c', 'o', 'm', 0,
        0, 1, 0, 1
    };
    // clang-format on

    size_t packetSize = sizeof(DnsHeader) + sizeof(questions);
    uint8_t* packet = new uint8_t[packetSize];

    DnsHeader* header = new (packet) DnsHeader{};
    header->setId(0x1234);
    header->setOpcode(DnsOpcode::Query);
    header->setRecursionDesired();
    header->setQdcount(1);

    println("recursion desired: {}", header->recursionDesired());
    println("flags: {:016b}", header->flags());

    memcpy(packet + sizeof(DnsHeader), questions, sizeof(questions));
    println("sending");
    udpSend(netif, dnsServer, 10053, 53, packet, packetSize, true);
    println("sent");

    delete[] packet;
    return IpAddress();
}

void dnsRecv(NetworkInterface* netif, uint8_t* buffer, size_t size) {
    if (size < sizeof(DnsHeader)) {
        return;
    }

    DnsHeader* header = reinterpret_cast<DnsHeader*>(buffer);
    if (!header->isReply()) {
        return;
    }

    println("dns response: id={}", header->id());
    println("dns response: opcode={}", (uint8_t)header->opcode());
    println("dns response: rcode={}", (uint8_t)header->rcode());
    println("dns response: qdcount={}", header->qdcount());
    println("dns response: ancount={}", header->ancount());
    println("dns response: nscount={}", header->nscount());
    println("dns response: arcount={}", header->arcount());
    println("dns response: flags={:016b}", header->flags());
    println("dns response: isReply={}", header->isReply());
    println("dns response: authoritativeAnswer={}", header->authoritativeAnswer());
    println("dns response: truncated={}", header->truncated());
    println("dns response: recursionDesired={}", header->recursionDesired());
    println("dns response: recursionAvailable={}", header->recursionAvailable());
}
