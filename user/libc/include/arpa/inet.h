#pragma once
#include <stdint.h>

extern "C" {
uint16_t ntohs(uint16_t value);
uint32_t ntohl(uint32_t value);
uint16_t htons(uint16_t value);
uint32_t htonl(uint32_t value);
}
