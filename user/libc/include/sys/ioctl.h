#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int fd, int op, void* argp);

#ifdef __cplusplus
}
#endif
