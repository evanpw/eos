// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
#pragma once

// Contains defines for each errno
#include "api/errno.h"

// TODO: should be thread-local?
extern "C" int errno;
