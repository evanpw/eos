#pragma once

struct Process;

struct Thread {
    Process& process;

    static Thread* s_current;
    static Thread& current() { return *s_current; }
};
