#include "timer.h"

#include "estd/print.h"
#include "interrupts.h"
#include "io.h"

uint64_t Timer::s_tickCount;

// Timer IRQ
void __attribute__((interrupt)) irqHandler0(InterruptFrame* frame) {
    Timer::increment();
    outb(PIC1_COMMAND, EOI);
}
