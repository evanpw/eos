#include <errno.h>
#include <sys/wait.h>

#include "syscall.h"

pid_t waitpid(pid_t pid, int* /*status*/, int /*options*/) {
    int result = __syscall(SYS_wait_pid, pid);
    if (result < 0) {
        errno = -result;
        return -1;
    }

    return pid;
}
