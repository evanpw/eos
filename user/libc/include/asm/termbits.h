#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Terminal ioctls
enum {
    TCGETS = 0x5401,
    TCSETS = 0x5402,
};

#ifdef __cplusplus
}
#endif
