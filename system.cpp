#include "system.h"

#include "interrupts.h"
#include "keyboard.h"
#include "mem.h"
#include "print.h"
#include "screen.h"

System* System::_instance = nullptr;

System::System() {
    ASSERT(_instance == nullptr);
    _instance = this;

    _screen = new Screen;
    _keyboard = new KeyboardDevice;
    installInterrupts();

    screen().clear(Screen::LightGreen);
}
