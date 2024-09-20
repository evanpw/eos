// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/dirent.h.html
#pragma once

#include <stdint.h>

// Defines struct dirent
#include "api/dirent.h"

extern "C" {

struct DIR {
    uint8_t buffer[4096];
    uint32_t bufferSize = 4096;
    uint32_t offset = 0;
};

int closedir(DIR*);
DIR* opendir(const char* name);
dirent* readdir(DIR*);
void rewinddir(DIR*);
void seekdir(DIR*, long loc);
long telldir(DIR*);
}
