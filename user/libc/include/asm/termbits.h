#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Terminal ioctls
enum {
    TCGETS = 0x5401,
    TCSETS = 0x5402,
    TIOCGWINSZ = 0x5413,
    TIOCSWINSZ = 0x5414,
};

#ifdef __cplusplus
}
#endif
