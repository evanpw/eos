#pragma once
#include <stdint.h>

#include "api/syscalls.h"  // IWYU pragma: export
#include "errno.h"         // IWYU pragma: keep

int64_t __syscall(uint64_t function, uint64_t arg1 = 0, uint64_t arg2 = 0,
                  uint64_t arg3 = 0, uint64_t arg4 = 0, uint64_t arg5 = 0,
                  uint64_t arg6 = 0);

template <typename Arg1 = uint64_t, typename Arg2 = uint64_t, typename Arg3 = uint64_t,
          typename Arg4 = uint64_t, typename Arg5 = uint64_t, typename Arg6 = uint64_t>
inline int64_t syscall(uint64_t function, Arg1 arg1 = 0, Arg2 arg2 = 0, Arg3 arg3 = 0,
                       Arg4 arg4 = 0, Arg5 arg5 = 0, Arg6 arg6 = 0) {
    return __syscall(function, (uint64_t)arg1, (uint64_t)arg2, (uint64_t)arg3,
                     (uint64_t)arg4, (uint64_t)arg5, (uint64_t)arg6);
}

template <typename R = int64_t, typename Arg1 = uint64_t, typename Arg2 = uint64_t,
          typename Arg3 = uint64_t, typename Arg4 = uint64_t, typename Arg5 = uint64_t,
          typename Arg6 = uint64_t>
inline R try_syscall(uint64_t function, Arg1 arg1 = 0, Arg2 arg2 = 0, Arg3 arg3 = 0,
                     Arg4 arg4 = 0, Arg5 arg5 = 0, Arg6 arg6 = 0) {
    int64_t result = __syscall(function, (uint64_t)arg1, (uint64_t)arg2, (uint64_t)arg3,
                               (uint64_t)arg4, (uint64_t)arg5, (uint64_t)arg6);

    if (result < 0) {
        errno = -result;
        return (R)-1;
    }

    return (R)result;
}
