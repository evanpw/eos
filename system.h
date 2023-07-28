// Holder class for initializing and accessing various components of the kernel
#pragma once
#include "estd/ownptr.h"
#include "mem.h"

class Screen;
class KeyboardDevice;
class Terminal;
class PCIDevices;
class IDEController;
class Ext2FileSystem;

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

    static PCIDevices& pciDevices() {
        ASSERT(instance()._pciDevices);
        return *(instance()._pciDevices);
    }

    static Ext2FileSystem& fs() {
        ASSERT(instance()._fs);
        return *(instance()._fs);
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
    OwnPtr<Terminal> _terminal;
    OwnPtr<PCIDevices> _pciDevices;
    OwnPtr<IDEController> _ideController;
    OwnPtr<Ext2FileSystem> _fs;
};
