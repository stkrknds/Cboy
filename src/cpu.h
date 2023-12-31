#ifndef CPU_H
#define CPU_H

#include "bus.h"
#include "instructions.h"
#include "ppu.h"
#include "timers.h"
#include "types.h"

typedef struct _cpu {
    // registers
    u8 F, A, C, B, E, D, L, H;
    u16 SP, PC;

    bool isHalted;
    bool isHaltBug;
    bool isCB;
    bool IME;
    bool scheduledIME;
} cpu;

typedef enum FLAG {
    // use flag as index
    Z = 7,
    N = 6,
    H = 5,
    C = 4
} FLAG;

extern u8 IE_register;
extern u8 IF_register;
#ifdef TEST_CHECK
extern bool testPassed;
#endif

void cpu_init();
#ifdef DEBUG
void cpu_run(FILE *logFile);
#else
void cpu_run();
#endif

#endif
