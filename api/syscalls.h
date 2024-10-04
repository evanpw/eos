#pragma once
#include <stddef.h>

#define SYS_read 0
#define SYS_write 1
#define SYS_getpid 2
#define SYS_exit 3
#define SYS_sleep 4
#define SYS_open 5
#define SYS_close 6
#define SYS_launch 7
#define SYS_read_dir 8
#define SYS_sbrk 9
#define SYS_getcwd 10
#define SYS_chdir 11
#define SYS_wait_pid 12

#define MAX_SYSCALL_NO 12
