#pragma once

#include <stddef.h>
#include <stdint.h>

struct MacAddress;
struct IpAddress;

class NicDevice {
public:
    virtual ~NicDevice() = default;

    virtual const MacAddress& macAddress() const;
    virtual const IpAddress& ipAddress() const;

    virtual void sendPacket(uint8_t* buffer, size_t length);
};
