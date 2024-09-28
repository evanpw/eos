#include "dhcp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/print.h"
#include "net/ip.h"
#include "net/nic_device.h"
#include "net/udp.h"

DhcpOperation DhcpHeader::op() { return (DhcpOperation)_op; }
DhcpHardwareType DhcpHeader::htype() { return (DhcpHardwareType)_htype; }
uint8_t DhcpHeader::hlen() { return _hlen; }
uint8_t DhcpHeader::hops() { return _hops; }
uint32_t DhcpHeader::xid() { return ntohl(_xid); }
uint16_t DhcpHeader::secs() { return ntohs(_secs); }
uint16_t DhcpHeader::flags() { return ntohl(_flags); }
IpAddress DhcpHeader::ciaddr() { return IpAddress(_ciaddr); }
IpAddress DhcpHeader::yiaddr() { return IpAddress(_yiaddr); }
IpAddress DhcpHeader::siaddr() { return IpAddress(_siaddr); }
IpAddress DhcpHeader::giaddr() { return IpAddress(_giaddr); }
MacAddress DhcpHeader::chaddr() { return MacAddress(_chaddr); }
const char* DhcpHeader::sname() { return _sname; }
const char* DhcpHeader::file() { return _file; }

bool DhcpHeader::checkMagic() { return _magic == DHCP_MAGIC; }
void DhcpHeader::fillMagic() { _magic = DHCP_MAGIC; }

uint8_t* DhcpHeader::findOption(DhcpOption code) {
    ASSERT(code != DhcpOption::Pad && code != DhcpOption::End);

    // TODO: bounds checking
    for (uint8_t* p = _options; *p != (uint8_t)DhcpOption::End;) {
        if (*p == (uint8_t)code) {
            return p;
        }

        // Options 0 (pad) and 255 (end) have no length are one byte long. All other
        // options' length are given by the following byte
        if (*p == (uint8_t)DhcpOption::Pad) {
            p += 1;
        } else {
            p += p[1] + 2;
        }
    }

    return nullptr;
}

bool DhcpHeader::subnetMask(IpAddress* subnetMask) {
    if (uint8_t* p = findOption(DhcpOption::SubnetMask)) {
        memcpy(subnetMask, p + 2, sizeof(IpAddress));
        return true;
    }

    return false;
}

estd::vector<IpAddress> DhcpHeader::routers() {
    uint8_t* start = findOption(DhcpOption::Router);
    uint8_t length = start[1];

    estd::vector<IpAddress> routers;
    for (uint8_t* p = start + 2; p < start + 2 + length; p += 4) {
        IpAddress router;
        memcpy(&router, p, sizeof(IpAddress));
        routers.push_back(router);
    }

    return estd::move(routers);
}

estd::vector<IpAddress> DhcpHeader::dnsServers() {
    uint8_t* start = findOption(DhcpOption::Dns);
    uint8_t length = start[1];

    estd::vector<IpAddress> dnsServers;
    for (uint8_t* p = start + 2; p < start + 2 + length; p += 4) {
        IpAddress dnsServer;
        memcpy(&dnsServer, p, sizeof(IpAddress));
        dnsServers.push_back(dnsServer);
    }

    return estd::move(dnsServers);
}

bool DhcpHeader::broadcastAddress(IpAddress* broadcastAddress) {
    if (uint8_t* p = findOption(DhcpOption::BroadcastAddress)) {
        memcpy(broadcastAddress, p + 2, sizeof(IpAddress));
        return true;
    }

    return false;
}

DhcpMessageType DhcpHeader::messageType() {
    if (uint8_t* p = findOption(DhcpOption::MessageType)) {
        return (DhcpMessageType)p[2];
    }

    return DhcpMessageType::Invalid;
}

void DhcpHeader::setOp(DhcpOperation value) { _op = (uint8_t)value; }
void DhcpHeader::setHtype(DhcpHardwareType value) { _htype = (uint8_t)value; }
void DhcpHeader::setHlen(uint8_t value) { _hlen = value; }
void DhcpHeader::setHops(uint8_t value) { _hops = value; }
void DhcpHeader::setXid(uint32_t value) { _xid = htonl(value); }
void DhcpHeader::setSecs(uint16_t value) { _secs = htons(value); }
void DhcpHeader::setFlags(uint16_t value) { _flags = htons(value); }
void DhcpHeader::setCiaddr(IpAddress value) { _ciaddr = value; }
void DhcpHeader::setYiaddr(IpAddress value) { _yiaddr = value; }
void DhcpHeader::setSiaddr(IpAddress value) { _siaddr = value; }
void DhcpHeader::setGiaddr(IpAddress value) { _giaddr = value; }
void DhcpHeader::setSname(const char* value) { strcpy(_sname, value); }
void DhcpHeader::setFile(const char* value) { strcpy(_file, value); }
void DhcpHeader::setOptions(uint8_t* data, size_t size) { memcpy(_options, data, size); }

void DhcpHeader::setChaddr(MacAddress value) {
    memset(_chaddr, 0, sizeof(_chaddr));
    memcpy(_chaddr, &value, sizeof(MacAddress));
}

estd::vector<uint8_t> DhcpHeader::createDiscoverOptions() {
    estd::vector<uint8_t> options;
    options.push_back((uint8_t)DhcpOption::MessageType);
    options.push_back(1);
    options.push_back((uint8_t)DhcpMessageType::Discover);
    options.push_back((uint8_t)DhcpOption::End);
    return estd::move(options);
}

void dhcpRecv(NicDevice* nic, IpHeader*, uint8_t* buffer, size_t size) {
    if (size < sizeof(DhcpHeader)) {
        return;
    }

    DhcpHeader* dhcpHeader = reinterpret_cast<DhcpHeader*>(buffer);
    if (!dhcpHeader->checkMagic()) return;
    if (dhcpHeader->op() != DhcpOperation::BootReply) return;
    if (dhcpHeader->htype() != DhcpHardwareType::Ethernet) return;
    if (dhcpHeader->hlen() != sizeof(MacAddress)) return;
    if (dhcpHeader->xid() != 0x12345678) return;
    if (dhcpHeader->messageType() != DhcpMessageType::Offer) return;
    if (dhcpHeader->chaddr() != nic->macAddress()) return;

    print("dhcp: inet ");
    dhcpHeader->yiaddr().print();

    IpAddress subnetMask;
    if (dhcpHeader->subnetMask(&subnetMask)) {
        print(" netmask ");
        subnetMask.print();
    }

    estd::vector<IpAddress> routers = dhcpHeader->routers();
    if (!routers.empty()) {
        print(" gateway ");
        routers[0].print();
    }

    println("");
}

void dhcpRequest(NicDevice* nic) {
    estd::vector<uint8_t> options = DhcpHeader().createDiscoverOptions();

    size_t packetSize = sizeof(DhcpHeader) + options.size();
    uint8_t* packet = new uint8_t[packetSize];

    DhcpHeader* request = new (packet) DhcpHeader;
    request->setOp(DhcpOperation::BootRequest);
    request->setHtype(DhcpHardwareType::Ethernet);
    request->setHlen(sizeof(MacAddress));
    request->setHops(0);
    request->setXid(0x12345678);
    request->setSecs(0);
    request->setFlags(0);
    request->setCiaddr(IpAddress());
    request->setYiaddr(IpAddress());
    request->setSiaddr(IpAddress());
    request->setGiaddr(IpAddress());
    request->setChaddr(nic->macAddress());
    request->setSname("");
    request->setFile("");
    request->fillMagic();
    request->setOptions(options.data(), options.size());

    udpSend(nic, IpAddress::broadcast(), 68, 67, packet, packetSize);
}
