// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/wait.h.html
#pragma once

#include <sys/types.h>

pid_t waitpid(pid_t pid, int* status, int options);
