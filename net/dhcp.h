// RFC 2131: Dynamic Host Configuration Protocol
// https://www.rfc-editor.org/rfc/rfc2131
//
// RFC 1533: DHCP Options and BOOTP Vendor Extensions
// https://www.rfc-editor.org/rfc/rfc1533
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/vector.h"
#include "net/ethernet.h"

class NicDevice;
struct IpAddress;
struct IpHeader;

enum class DhcpHardwareType : uint8_t {
    Ethernet = 1,
};

enum class DhcpOperation : uint8_t {
    BootRequest = 1,
    BootReply = 2,
};

enum class DhcpMessageType : uint8_t {
    Invalid = 0,
    Discover = 1,
    Offer = 2,
    Request = 3,
    Decline = 4,
    Ack = 5,
    Nak = 6,
    Release = 7,
    Inform = 8,
};

// Network byte order
static constexpr uint32_t DHCP_MAGIC = 0x63538263;

enum class DhcpOption : uint8_t {
    Pad = 0,
    End = 255,
    SubnetMask = 1,
    Router = 3,
    Dns = 6,
    BroadcastAddress = 28,
    MessageType = 53,
};

class DhcpHeader {
    uint8_t _op;     // message op code / message type
    uint8_t _htype;  // hardware address type
    uint8_t _hlen;   // hardware address length
    uint8_t _hops;   // relay agent hops
    uint32_t _xid;   // transaction id
    uint16_t _secs;  // seconds since client started began acquisition / renewal
    uint16_t _flags;
    uint32_t _ciaddr;     // client ip address
    uint32_t _yiaddr;     // 'your' client ip address
    uint32_t _siaddr;     // server ip address
    uint32_t _giaddr;     // relay agent ip address
    uint8_t _chaddr[16];  // client hardware address
    char _sname[64];      // server host name
    char _file[128];      // boot file name
    uint32_t _magic;      // magic cookie
    uint8_t _options[];   // optional parameters field

    uint8_t* findOption(DhcpOption code);

public:
    DhcpOperation op();
    DhcpHardwareType htype();
    uint8_t hlen();
    uint8_t hops();
    uint32_t xid();
    uint16_t secs();
    uint16_t flags();
    IpAddress ciaddr();
    IpAddress yiaddr();
    IpAddress siaddr();
    IpAddress giaddr();
    MacAddress chaddr();
    const char* sname();
    const char* file();

    void setOp(DhcpOperation value);
    void setHtype(DhcpHardwareType value);
    void setHlen(uint8_t value);
    void setHops(uint8_t value);
    void setXid(uint32_t value);
    void setSecs(uint16_t value);
    void setFlags(uint16_t value);
    void setCiaddr(IpAddress value);
    void setYiaddr(IpAddress value);
    void setSiaddr(IpAddress value);
    void setGiaddr(IpAddress value);
    void setChaddr(MacAddress value);
    void setSname(const char* value);
    void setFile(const char* value);
    void setOptions(uint8_t* data, size_t size);

    bool checkMagic();
    void fillMagic();

    // Options
    bool subnetMask(IpAddress* subnetMask);
    estd::vector<IpAddress> routers();
    estd::vector<IpAddress> dnsServers();
    bool broadcastAddress(IpAddress* broadcastAddress);
    DhcpMessageType messageType();

    estd::vector<uint8_t> createDiscoverOptions();
};

void dhcpRecv(NicDevice* nic, IpHeader* ipHeader, uint8_t* buffer, size_t size);
void dhcpRequest(NicDevice* nic);
