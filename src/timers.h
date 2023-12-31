#ifndef TIMERS_H
#define TIMERS_H

#include "types.h"

#include <stdbool.h>

typedef enum TIMAstate { NORMAL, OVERFLOW, ABORTED_OVERFLOW } TIMAstate;

typedef struct TIMA {
    u8 reg;
    u8 cntOverflowCycles;

    u16 mask;

    bool AND_result;
    bool enable;

    TIMAstate state;
} TIMA;

u8 timers_read(u16 addr);
void timers_write(u16 addr, u8 val);
void timers_init();
void timers_tick();

extern TIMA tima;
extern u16 DIV_register;
extern u8 TMA_register;
extern u8 TAC_register;
#endif // TIMERS_H
