// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/dirent.h.html
#pragma once

#include <stdint.h>

// Defines struct dirent
#include "api/dirent.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DIR {
    uint8_t* buffer;
    uint32_t bufferSize;
    uint32_t offset;
};

int closedir(struct DIR*);
DIR* opendir(const char* name);
dirent* readdir(struct DIR*);
void rewinddir(struct DIR*);
void seekdir(struct DIR*, long loc);
long telldir(struct DIR*);

#ifdef __cplusplus
}
#endif
