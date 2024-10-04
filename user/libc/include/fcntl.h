// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/fcntl.h.html
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int open(const char* path, int oflag);
int close(int fd);

#ifdef __cplusplus
}
#endif
