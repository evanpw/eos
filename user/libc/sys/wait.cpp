#include <sys/wait.h>

#include "syscall.h"

pid_t waitpid(pid_t pid, int* /*status*/, int /*options*/) {
    return try_syscall(SYS_wait_pid, pid);
}
