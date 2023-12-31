#include "timing.h"
#include "ppu.h"
#include "timers.h"

void tick_TCycles(uint num_cycles) {
    for (uint i = 0; i < num_cycles; i++) {
        timers_tick();
        ppu_tick();
    }
}

void tick_MCycle() { tick_TCycles(4); }
