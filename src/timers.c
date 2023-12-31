#include "timers.h"
#include "bus.h"
#include "cpu.h"

TIMA tima;
u16 DIV_register;
u8 TMA_register;
u8 TAC_register;

static const u16 TACmasks[4] = {[0x00] = 0x0200, [0x01] = 0x0008, [0x02] = 0x0020, [0x03] = 0x0080};

static void TIMA_write(TIMA *tima, u8 val) {
    if (tima->cntOverflowCycles < 4) {
        tima->state = NORMAL;
        tima->reg = val;
        tima->cntOverflowCycles = 0;
    }
}

static void TIMA_setAfterTAC(u8 TAC_register, TIMA *tima) {
    tima->mask = TACmasks[TAC_register & 0x03];
    tima->enable = bit_read(TAC_register, 2);
}

static void TAC_write(u8 *TAC_register, u8 val) { *TAC_register = (*TAC_register & 0xF8) | (val & 0x07); }

// credits: https://github.com/pmcanseco/java-gb/blob/master/src/main/java/TimerService.java
// TODO proper implementation
static void TIMA_tick(TIMA *tima) {
    bool new_AND_result;
    bool DIV_bit;

    DIV_bit = DIV_register & tima->mask;
    new_AND_result = tima->enable && DIV_bit;

    switch (tima->state) {
        case NORMAL:
            if (!new_AND_result && tima->AND_result) {
                if (tima->reg == 0xFF) {
                    tima->reg = 0;
                    tima->state = OVERFLOW;
                }
                else
                    tima->reg++;
            }
            break;
        case OVERFLOW:
            tima->cntOverflowCycles++;
            if (tima->cntOverflowCycles == 4) {
                tima->reg = TMA_register;
                bit_set(&IF_register, 2);
            }
            else if (tima->cntOverflowCycles == 5) {
                tima->reg = TMA_register;
                tima->state = NORMAL;
                tima->cntOverflowCycles = 0;
            }
            break;
        case ABORTED_OVERFLOW:
            tima->cntOverflowCycles++;
            if (tima->cntOverflowCycles == 4) {
                tima->cntOverflowCycles = 0;
                tima->state = NORMAL;
            }
            break;
    }
    tima->AND_result = new_AND_result;
}

u8 timers_read(u16 addr) {
    u8 val = 0xFF;
    switch (addr) {
        case 0xFF03:
            val = u16_lsb(&DIV_register);
            break;
        case 0xFF04:
            val = u16_msb(&DIV_register);
            break;
        case 0xFF05:
            val = tima.reg;
            break;
        case 0xFF06:
            val = TMA_register;
            break;
        case 0xFF07:
            val = TAC_register & 0x07;
            break;
    }
    return val;
}

void timers_write(u16 addr, u8 val) {
    switch (addr) {
        case 0xFF03:
        case 0xFF04:
            DIV_register = 0;
            break;
        case 0xFF05:
            TIMA_write(&tima, val);
            break;
        case 0xFF06:
            TMA_register = val;
            break;
        case 0xFF07:
            TAC_write(&TAC_register, val);
            TIMA_setAfterTAC(TAC_register, &tima);
            TIMA_tick(&tima);
            break;
    }
}

void timers_init() {
    DIV_register = 0xAB00;
    tima.reg = 0;
    tima.state = NORMAL;
    tima.AND_result = 0;
    tima.cntOverflowCycles = 0;
    TMA_register = 0;
    TAC_register = 0xF8;
    TIMA_setAfterTAC(TAC_register, &tima);
}

void timers_tick() {
    DIV_register++;
    TIMA_tick(&tima);
}
