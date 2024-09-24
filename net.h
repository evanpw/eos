#pragma once
#include <stddef.h>
#include <stdint.h>

class E1000Device;

struct __attribute__((packed)) MacAddress {
    uint8_t bytes[6];

    const uint8_t& operator[](size_t index) const { return bytes[index]; }
    uint8_t& operator[](size_t index) { return bytes[index]; }
};

struct __attribute__((packed)) IpAddress {
    uint8_t bytes[4];

    const uint8_t& operator[](size_t index) const { return bytes[index]; }
    uint8_t& operator[](size_t index) { return bytes[index]; }
};

void printMacAddress(MacAddress mac);
void printIpAddress(IpAddress ip);

void handlePacket(E1000Device* nic, uint8_t* buffer, size_t size);
