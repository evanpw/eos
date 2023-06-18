#pragma once
#include <stddef.h>

static constexpr size_t SYS_read = 0;
static constexpr size_t SYS_write = 1;
static constexpr size_t SYS_getpid = 2;
static constexpr size_t SYS_exit = 3;

static constexpr size_t MAX_SYSCALL_NO = 3;
