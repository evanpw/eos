#pragma once

#include <stddef.h>
#include <stdint.h>

#include "estd/assertions.h"
#include "net/ethernet.h"
#include "net/ip.h"

class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;

    virtual MacAddress macAddress() const { return _macAddress; }

    virtual bool isConfigured() const { return _isConfigured; }
    virtual IpAddress ipAddress() const { return _ipAddress; }
    virtual IpAddress subnetMask() const { return _subnetMask; }
    virtual IpAddress gateway() const { return _gateway; }

    virtual bool isLocal(IpAddress ip) const {
        ASSERT(_isConfigured);
        return (ip & _subnetMask) == (_ipAddress & _subnetMask);
    }

    virtual void configure(IpAddress ipAddress, IpAddress subnetMask, IpAddress gateway) {
        _isConfigured = true;
        _ipAddress = ipAddress;
        _subnetMask = subnetMask;
        _gateway = gateway;
    }

    virtual void sendPacket(uint8_t* buffer, size_t length) = 0;

protected:
    MacAddress _macAddress;

    bool _isConfigured = false;
    IpAddress _ipAddress;
    IpAddress _subnetMask;
    IpAddress _gateway;
};
