// Holder class for initializing and accessing various components of the kernel
#pragma once
#include "estd/memory.h"
#include "mem.h"

class Screen;
class KeyboardDevice;
class Terminal;
class PCIDevices;
class IDEController;
class Ext2FileSystem;
struct Scheduler;
class Timer;
class NicDevice;

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

    static estd::shared_ptr<Terminal> terminal();

    static PCIDevices& pciDevices() {
        ASSERT(instance()._pciDevices);
        return *(instance()._pciDevices);
    }

    static Ext2FileSystem& fs() {
        ASSERT(instance()._fs);
        return *(instance()._fs);
    }

    static NicDevice& nic() {
        ASSERT(instance()._nic);
        return *(instance()._nic);
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

    estd::unique_ptr<Screen> _screen;
    estd::unique_ptr<KeyboardDevice> _keyboard;
    estd::shared_ptr<Terminal> _terminal;
    estd::unique_ptr<PCIDevices> _pciDevices;
    estd::unique_ptr<IDEController> _ideController;
    estd::unique_ptr<NicDevice> _nic;
    estd::unique_ptr<Ext2FileSystem> _fs;
    estd::unique_ptr<Scheduler> _scheduler;
    estd::unique_ptr<Timer> _timer;
};
