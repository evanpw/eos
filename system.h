// Holder class for initializing and accessing various components of the kernel
#pragma once
#include "estd/ownptr.h"
#include "estd/sharedptr.h"
#include "mem.h"

class Screen;
class KeyboardDevice;
class Terminal;
class PCIDevices;
class IDEController;
class Ext2FileSystem;
struct Scheduler;
class Timer;

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

    static SharedPtr<Terminal> terminal();

    static PCIDevices& pciDevices() {
        ASSERT(instance()._pciDevices);
        return *(instance()._pciDevices);
    }

    static Ext2FileSystem& fs() {
        ASSERT(instance()._fs);
        return *(instance()._fs);
    }

    static Scheduler& scheduler() {
        ASSERT(instance()._scheduler);
        return *(instance()._scheduler);
    }

    static Timer& timer() {
        ASSERT(instance()._timer);
        return *(instance()._timer);
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

    OwnPtr<Screen> _screen;
    OwnPtr<KeyboardDevice> _keyboard;
    SharedPtr<Terminal> _terminal;
    OwnPtr<PCIDevices> _pciDevices;
    OwnPtr<IDEController> _ideController;
    OwnPtr<Ext2FileSystem> _fs;
    OwnPtr<Scheduler> _scheduler;
    OwnPtr<Timer> _timer;
};
