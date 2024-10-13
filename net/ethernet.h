#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/print.h"
#include "estd/stddef.h"

class NetworkInterface;

struct MacAddress {
    byte bytes[6] = {};

    MacAddress() = default;
    MacAddress(byte* bytes);

    MacAddress(const MacAddress& other);
    MacAddress& operator=(const MacAddress& other);

    bool operator==(const MacAddress& other) const;

    static MacAddress broadcast();
};

static_assert(sizeof(MacAddress) == 6);

// Custom formatter for estd::print
template <>
struct FormatArg<MacAddress> : public FormatArgBase {
    FormatArg(const MacAddress& value) : value(value) {}

    void print(const FormatSpec&) const override {
        FormatSpec spec = {.base = 16, .padTo = 2, .padChar = '0', .uppercase = true};
        printInt(spec, value.bytes[0]);
        printChar(':');
        printInt(spec, value.bytes[1]);
        printChar(':');
        printInt(spec, value.bytes[2]);
        printChar(':');
        printInt(spec, value.bytes[3]);
        printChar(':');
        printInt(spec, value.bytes[4]);
        printChar(':');
        printInt(spec, value.bytes[5]);
    }

private:
    const MacAddress& value;
};

enum class EtherType : uint16_t {
    // Values are in network byte order
    Ipv4 = 0x0008,
    Arp = 0x0608,
};

class EthernetHeader {
    byte _destMac[6];
    byte _srcMac[6];
    uint16_t _etherType;

public:
    MacAddress destMac();
    MacAddress srcMac();
    EtherType etherType();

    void setDestMac(MacAddress value);
    void setSrcMac(MacAddress value);
    void setEtherType(EtherType value);
};

static_assert(sizeof(EthernetHeader) == 14);

void ethRecv(NetworkInterface* netif, void* buffer, size_t size);
void ethSend(NetworkInterface* netif, MacAddress destMac, EtherType ethType, void* buffer,
             size_t size);
