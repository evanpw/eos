#pragma once
#include <stdint.h>

using SyscallHandler = int64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t, uint64_t);

int64_t sys_add(int64_t a, int64_t b);
int64_t sys_print(int64_t arg);
int64_t sys_add6(int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4,
                 int64_t arg5, int64_t arg6);

static constexpr uint64_t MAX_SYSCALL_NO = 2;

extern "C" [[gnu::naked]] void syscallEntry();

void initSyscalls();
