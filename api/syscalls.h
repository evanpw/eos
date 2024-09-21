#pragma once
#include <stddef.h>

static constexpr size_t SYS_read = 0;
static constexpr size_t SYS_write = 1;
static constexpr size_t SYS_getpid = 2;
static constexpr size_t SYS_exit = 3;
static constexpr size_t SYS_sleep = 4;
static constexpr size_t SYS_open = 5;
static constexpr size_t SYS_close = 6;
static constexpr size_t SYS_launch = 7;
static constexpr size_t SYS_read_dir = 8;
static constexpr size_t SYS_sbrk = 9;
static constexpr size_t SYS_getcwd = 10;
static constexpr size_t SYS_chdir = 11;
static constexpr size_t SYS_wait_pid = 12;

static constexpr size_t MAX_SYSCALL_NO = 12;
