// Thread class
#pragma once

#include "address.h"
#include "trap.h"

struct Process;

struct Thread {
    Thread(Process& process, VirtualAddress entryPoint);

    Process& process;
    VirtualAddress stackTop;
    TrapRegisters regs;

    static Thread* s_current;
    static Thread& current() { return *s_current; }
};
