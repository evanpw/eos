#pragma once
#include "mem.h"

class Screen;
class KeyboardDevice;
class Terminal;

// Holder class for initializing and accessing various components
// of the kernel
class System {
public:
    static void run();

    static MemoryManager& mm() { return instance()._mm; }

    static Screen& screen() {
        ASSERT(instance()._screen);
        return *(instance()._screen);
    }

    static KeyboardDevice& keyboard() {
        ASSERT(instance()._keyboard);
        return *(instance()._keyboard);
    }

    static Terminal& terminal() {
        ASSERT(instance()._terminal);
        return *(instance()._terminal);
    }

private:
    System();

    static System& instance() {
        ASSERT(_instance);
        return *_instance;
    }

    static System* _instance;

    // This can't be a pointer because we need it to implement new()
    MemoryManager _mm;

    Screen* _screen = nullptr;
    KeyboardDevice* _keyboard = nullptr;
    Terminal* _terminal = nullptr;
};
