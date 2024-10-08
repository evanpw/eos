#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "estd/optional.h"
#include "estd/print.h"
#include "net/ethernet.h"

class NetworkInterface;

struct IpAddress {
    // Network byte order
    uint32_t value = 0;

    IpAddress() = default;
    IpAddress(uint32_t value) : value(value) {}
    IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    operator uint32_t() const { return value; }
    explicit operator bool() const { return value != 0; }
    bool operator==(const IpAddress& other) const { return value == other.value; }

    static IpAddress broadcast() { return IpAddress(0xFFFFFFFF); }
};

static_assert(sizeof(IpAddress) == 4);

// Custom formatter for estd::print
template <>
struct FormatArg<IpAddress> : public FormatArgBase {
    FormatArg(IpAddress value) : value(value) {}

    void print(const FormatSpec&) const override {
        uint8_t bytes[4];
        memcpy(bytes, &value, sizeof(uint32_t));

        FormatSpec spec;
        printInt(spec, bytes[0]);
        printChar('.');
        printInt(spec, bytes[1]);
        printChar('.');
        printInt(spec, bytes[2]);
        printChar('.');
        printInt(spec, bytes[3]);
    }

private:
    IpAddress value;
};

enum class IpProtocol : uint8_t {
    Tcp = 6,
    Udp = 17,
};

class IpHeader {
    uint8_t _headerLen : 4 = 5;
    uint8_t _version : 4 = 4;
    uint8_t _ecn : 2 = 0;
    uint8_t _dscp : 6 = 0;
    uint16_t _totalLen;
    uint16_t _identification = 0;
    uint16_t _fragmentOffset : 13 = 0;
    uint16_t _flags : 3 = 0;
    uint8_t _ttl = 255;
    uint8_t _protocol;
    uint16_t _checksum;
    uint32_t _sourceIp;
    uint32_t _destIp;

    uint16_t computeChecksum();

public:
    bool verifyChecksum();
    void fillChecksum();

    uint8_t headerLen();
    uint8_t version();
    uint8_t ecn();
    uint8_t dscp();
    uint16_t totalLen();
    uint16_t identification();
    uint16_t fragmentOffset();
    uint16_t flags();
    uint8_t ttl();
    IpProtocol protocol();
    IpAddress sourceIp();
    IpAddress destIp();

    void setTotalLen(uint16_t value);
    void setProtocol(IpProtocol value);
    void setSourceIp(IpAddress value);
    void setDestIp(IpAddress value);
};

static_assert(sizeof(IpHeader) == 20);

void ipInit();
estd::optional<IpAddress> findRouteSourceIp(IpAddress destIp);
void ipRecv(NetworkInterface* netif, uint8_t* buffer, size_t size);
bool ipBroadcast(NetworkInterface* netif, IpProtocol protocol, void* buffer, size_t size);
bool ipSend(IpAddress destIp, IpProtocol protocol, void* buffer, size_t size);

// Callback from the arp layer
void ipRouteFound(IpAddress ip);
