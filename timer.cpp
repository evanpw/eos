#include "timer.h"

#include "estd/print.h"
#include "interrupts.h"
#include "io.h"
#include "scheduler.h"
#include "system.h"

uint64_t Timer::s_tickCount;

void Timer::init() {
    s_tickCount = 0;
    registerIrqHandler(0, irqHandler0);
}

// Timer IRQ
void irqHandler0(TrapRegisters&) {
    Timer::increment();
    outb(PIC1_COMMAND, EOI);
    System::scheduler().run();
}
