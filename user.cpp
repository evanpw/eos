#include "user.h"

#include "screen.h"

void userTask() {
    uint16_t* vram = (uint16_t*)0xB8000;
    vram[0] = (Screen::Black << 12) | (Screen::LightGrey << 8) | 'U';

    while (true)
        ;
}
