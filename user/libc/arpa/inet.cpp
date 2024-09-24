#include <arpa/inet.h>

uint16_t ntohs(uint16_t value) { return (value >> 8) | (value << 8); }

uint32_t ntohl(uint32_t value) {
    return (value >> 24) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) |
           (value << 24);
}

uint16_t htons(uint16_t value) { return ntohs(value); }

uint32_t htonl(uint32_t value) { return ntohl(value); }
