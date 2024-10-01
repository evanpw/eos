// Holder class for initializing and accessing various components of the kernel
#pragma once
#include "estd/memory.h"

class Screen;
class KeyboardDevice;
class Terminal;
class PCIDevices;
class IDEController;
class Ext2FileSystem;
struct Scheduler;
class Timer;
class NetworkInterface;

class System {
public:
    System();
    void run();

    Screen& screen() { return *_screen; }
    KeyboardDevice& keyboard() { return *_keyboard; }
    estd::shared_ptr<Terminal> terminal();
    PCIDevices& pciDevices() { return *_pciDevices; }
    Ext2FileSystem& fs() { return *_fs; }
    NetworkInterface& netif() { return *_netif; }
    Scheduler& scheduler() { return *_scheduler; }
    Timer& timer() { return *_timer; }

private:
    estd::unique_ptr<Screen> _screen;
    estd::unique_ptr<KeyboardDevice> _keyboard;
    estd::shared_ptr<Terminal> _terminal;
    estd::unique_ptr<PCIDevices> _pciDevices;
    estd::unique_ptr<IDEController> _ideController;
    estd::unique_ptr<NetworkInterface> _netif;
    estd::unique_ptr<Ext2FileSystem> _fs;
    estd::unique_ptr<Scheduler> _scheduler;
    estd::unique_ptr<Timer> _timer;
};

extern System sys;
