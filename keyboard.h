#pragma once
#include "interrupts.h"

void __attribute__((interrupt)) irqHandler1(InterruptFrame* frame);
void initializeKeyboard();
