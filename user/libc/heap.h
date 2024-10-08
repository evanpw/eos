#pragma once

#include <stdint.h>

extern "C" char* nextFreeAddress;
extern "C" char* heapEnd;
void initHeap();
