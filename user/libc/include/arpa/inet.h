#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t ntohs(uint16_t value);
uint32_t ntohl(uint32_t value);
uint16_t htons(uint16_t value);
uint32_t htonl(uint32_t value);

#ifdef __cplusplus
}
#endif
