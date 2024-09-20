#pragma once

#include <stdint.h>

extern "C" uint8_t* nextFreeAddress;
extern "C" uint8_t* heapEnd;
void initHeap();
