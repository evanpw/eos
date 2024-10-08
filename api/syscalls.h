#pragma once
#include <stddef.h>

enum {
    SYS_read,
    SYS_write,
    SYS_getpid,
    SYS_exit,
    SYS_sleep,
    SYS_open,
    SYS_close,
    SYS_launch,
    SYS_read_dir,
    SYS_sbrk,
    SYS_getcwd,
    SYS_chdir,
    SYS_wait_pid,
    SYS_socket,
    SYS_connect,
    SYS_send,
    SYS_recv,
    SYS_bind,
    SYS_listen,
    SYS_accept,

    SYS_COUNT,
};
