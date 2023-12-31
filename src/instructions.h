#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "types.h"

typedef enum instr_type {
    LD,
    LD_SP_HL,
    LDD,
    LDH,
    LDI,
    LDHL,
    PUSH,
    POP,
    ADD,
    ADD16,
    ADDSP,
    ADC,
    SUB,
    SBC,
    AND,
    OR,
    XOR,
    CP,
    INC,
    INC16,
    DEC,
    DEC16,
    DAA,
    CPL,
    CCF,
    SCF,
    NOP,
    HALT,
    STOP,
    DI,
    EI,
    RLCA,
    RLA,
    RRCA,
    RRA,
    JP,
    JR,
    CALL,
    RST,
    RET,
    RETI,
    CB,
    // CB instructions
    SWAP,
    RLC,
    RL,
    RRC,
    RR,
    SLA,
    SRA,
    SRL,
    BIT,
    SET,
    RES,
} instr_type;

typedef enum instr_op {
    REG_A,
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_H,
    REG_L,
    REG_SP,
    REG_PC,

    REG_AF,
    REG_BC,
    REG_DE,
    REG_HL,

    DATA_HL,
    DATA_BC,
    DATA_DE,
    DATA_NN,
    DATA_NN16,
    DATA_N,
    DATA_C,

    IM_DATA8,
    IM_DATA16,
    NONE,

    JP_NZ,
    JP_Z,
    JP_NC,
    JP_C,
    JP_HL,
} instr_op;

typedef struct instruction {
    instr_type type;
    instr_op op_a;
    instr_op op_b;
} instruction;

instruction opcode_to_instr(u8 op_code, bool isCB);

#endif
