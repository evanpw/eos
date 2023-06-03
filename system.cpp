#include "system.h"

#include "interrupts.h"
#include "keyboard.h"
#include "mem.h"
#include "print.h"
#include "screen.h"
#include "terminal.h"

void System::run() {
    System system;
    while (true)
        ;
}

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    _terminal = new Terminal(*_keyboard, *_screen);
    installInterrupts();
}
