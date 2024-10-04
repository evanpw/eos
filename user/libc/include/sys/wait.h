// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/wait.h.html
#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

pid_t waitpid(pid_t pid, int* status, int options);

#ifdef __cplusplus
}
#endif
