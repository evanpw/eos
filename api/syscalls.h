#pragma once

// This is made available to syscall_entry.S
#define MAX_SYSCALL_NO 4

#ifndef __ASSEMBLER__
#include <stddef.h>

static constexpr size_t SYS_read = 0;
static constexpr size_t SYS_write = 1;
static constexpr size_t SYS_getpid = 2;
static constexpr size_t SYS_exit = 3;
static constexpr size_t SYS_sleep = 4;
#endif
