// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/dirent.h.html
#pragma once

#include <stdint.h>

extern "C" {

struct __attribute__((packed)) dirent {
    uint32_t d_ino;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[];
};
}
