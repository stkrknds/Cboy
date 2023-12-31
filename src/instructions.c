#include "instructions.h"

#include <stdio.h>

#include "types.h"

static const instruction instructions[256] = {
    // LD r, data8
    [0x3E] = {LD, REG_A, IM_DATA8},
    [0x06] = {LD, REG_B, IM_DATA8},
    [0x0E] = {LD, REG_C, IM_DATA8},
    [0x16] = {LD, REG_D, IM_DATA8},
    [0x1E] = {LD, REG_E, IM_DATA8},
    [0x26] = {LD, REG_H, IM_DATA8},
    [0x2E] = {LD, REG_L, IM_DATA8},

    // LD r1, r2
    [0x7F] = {LD, REG_A, REG_A},
    [0x78] = {LD, REG_A, REG_B},
    [0x79] = {LD, REG_A, REG_C},
    [0x7A] = {LD, REG_A, REG_D},
    [0x7B] = {LD, REG_A, REG_E},
    [0x7C] = {LD, REG_A, REG_H},
    [0x7D] = {LD, REG_A, REG_L},
    [0x40] = {LD, REG_B, REG_B},
    [0x41] = {LD, REG_B, REG_C},
    [0x42] = {LD, REG_B, REG_D},
    [0x43] = {LD, REG_B, REG_E},
    [0x44] = {LD, REG_B, REG_H},
    [0x45] = {LD, REG_B, REG_L},
    [0x48] = {LD, REG_C, REG_B},
    [0x49] = {LD, REG_C, REG_C},
    [0x4A] = {LD, REG_C, REG_D},
    [0x4B] = {LD, REG_C, REG_E},
    [0x4C] = {LD, REG_C, REG_H},
    [0x4D] = {LD, REG_C, REG_L},
    [0x50] = {LD, REG_D, REG_B},
    [0x51] = {LD, REG_D, REG_C},
    [0x52] = {LD, REG_D, REG_D},
    [0x53] = {LD, REG_D, REG_E},
    [0x54] = {LD, REG_D, REG_H},
    [0x55] = {LD, REG_D, REG_L},
    [0x58] = {LD, REG_E, REG_B},
    [0x59] = {LD, REG_E, REG_C},
    [0x5A] = {LD, REG_E, REG_D},
    [0x5B] = {LD, REG_E, REG_E},
    [0x5C] = {LD, REG_E, REG_H},
    [0x5D] = {LD, REG_E, REG_L},
    [0x60] = {LD, REG_H, REG_B},
    [0x61] = {LD, REG_H, REG_C},
    [0x62] = {LD, REG_H, REG_D},
    [0x63] = {LD, REG_H, REG_E},
    [0x64] = {LD, REG_H, REG_H},
    [0x65] = {LD, REG_H, REG_L},
    [0x68] = {LD, REG_L, REG_B},
    [0x69] = {LD, REG_L, REG_C},
    [0x6A] = {LD, REG_L, REG_D},
    [0x6B] = {LD, REG_L, REG_E},
    [0x6C] = {LD, REG_L, REG_H},
    [0x6D] = {LD, REG_L, REG_L},
    [0x47] = {LD, REG_B, REG_A},
    [0x4F] = {LD, REG_C, REG_A},
    [0x57] = {LD, REG_D, REG_A},
    [0x5F] = {LD, REG_E, REG_A},
    [0x67] = {LD, REG_H, REG_A},
    [0x6F] = {LD, REG_L, REG_A},

    // LD r, (r16)
    [0x7E] = {LD, REG_A, DATA_HL},
    [0x46] = {LD, REG_B, DATA_HL},
    [0x4E] = {LD, REG_C, DATA_HL},
    [0x56] = {LD, REG_D, DATA_HL},
    [0x5E] = {LD, REG_E, DATA_HL},
    [0x66] = {LD, REG_H, DATA_HL},
    [0x6E] = {LD, REG_L, DATA_HL},
    [0x0A] = {LD, REG_A, DATA_BC},
    [0x1A] = {LD, REG_A, DATA_DE},

    // LD (r16), r
    [0x70] = {LD, DATA_HL, REG_B},
    [0x71] = {LD, DATA_HL, REG_C},
    [0x72] = {LD, DATA_HL, REG_D},
    [0x73] = {LD, DATA_HL, REG_E},
    [0x74] = {LD, DATA_HL, REG_H},
    [0x75] = {LD, DATA_HL, REG_L},
    [0x02] = {LD, DATA_BC, REG_A},
    [0x12] = {LD, DATA_DE, REG_A},
    [0x77] = {LD, DATA_HL, REG_A},

    // LD (r16), data8
    [0x36] = {LD, DATA_HL, IM_DATA8},

    // LD r, (data16)
    [0xFA] = {LD, REG_A, DATA_NN},

    // LD (data16), r
    [0xEA] = {LD, DATA_NN, REG_A},

    // LD r, (r + offset)
    [0xF2] = {LD, REG_A, DATA_C},

    // LD (r + offset), r
    [0xE2] = {LD, DATA_C, REG_A},

    // LDD
    [0x3A] = {LDD, REG_A, DATA_HL},
    [0x32] = {LDD, DATA_HL, REG_A},

    // LDI
    [0x2A] = {LDI, REG_A, DATA_HL},
    [0x22] = {LDI, DATA_HL, REG_A},

    // LD (data8 + offset), r
    [0xE0] = {LD, DATA_N, REG_A},

    // LD r, (data8 + offset)
    [0xF0] = {LD, REG_A, DATA_N},

    // LD16 r, data16
    [0x01] = {LD, REG_BC, IM_DATA16},
    [0x11] = {LD, REG_DE, IM_DATA16},
    [0x21] = {LD, REG_HL, IM_DATA16},
    [0x31] = {LD, REG_SP, IM_DATA16},

    // LD_SP_HL
    [0xF9] = {LD_SP_HL, REG_SP, REG_HL},

    // LD_HL
    [0xF8] = {LDHL, REG_SP, IM_DATA8},

    // LD16 (data16), r16
    [0x08] = {LD, DATA_NN16, REG_SP},

    // STACK
    // PUSH r16
    [0xF5] = {PUSH, REG_AF, NONE},
    [0xC5] = {PUSH, REG_BC, NONE},
    [0xD5] = {PUSH, REG_DE, NONE},
    [0xE5] = {PUSH, REG_HL, NONE},
    // POP r16
    [0xF1] = {POP, REG_AF, NONE},
    [0xC1] = {POP, REG_BC, NONE},
    [0xD1] = {POP, REG_DE, NONE},
    [0xE1] = {POP, REG_HL, NONE},

    // ADD r, r
    [0x87] = {ADD, REG_A, REG_A},
    [0x80] = {ADD, REG_A, REG_B},
    [0x81] = {ADD, REG_A, REG_C},
    [0x82] = {ADD, REG_A, REG_D},
    [0x83] = {ADD, REG_A, REG_E},
    [0x84] = {ADD, REG_A, REG_H},
    [0x85] = {ADD, REG_A, REG_L},
    // ADD r, (r16)
    [0x86] = {ADD, REG_A, DATA_HL},
    // ADD r, data8
    [0xC6] = {ADD, REG_A, IM_DATA8},

    // ADD16 r16, r16
    [0x09] = {ADD16, REG_HL, REG_BC},
    [0x19] = {ADD16, REG_HL, REG_DE},
    [0x29] = {ADD16, REG_HL, REG_HL},
    [0x39] = {ADD16, REG_HL, REG_SP},

    // ADDSP
    [0xE8] = {ADDSP, REG_SP, IM_DATA8},

    // ADC r, r
    [0x8F] = {ADC, REG_A, REG_A},
    [0x88] = {ADC, REG_A, REG_B},
    [0x89] = {ADC, REG_A, REG_C},
    [0x8A] = {ADC, REG_A, REG_D},
    [0x8B] = {ADC, REG_A, REG_E},
    [0x8C] = {ADC, REG_A, REG_H},
    [0x8D] = {ADC, REG_A, REG_L},
    // ADC r, (r16)
    [0x8E] = {ADC, REG_A, DATA_HL},
    // ADC r, data8
    [0xCE] = {ADC, REG_A, IM_DATA8},

    // SUB
    [0x97] = {SUB, REG_A, REG_A},
    [0x90] = {SUB, REG_A, REG_B},
    [0x91] = {SUB, REG_A, REG_C},
    [0x92] = {SUB, REG_A, REG_D},
    [0x93] = {SUB, REG_A, REG_E},
    [0x94] = {SUB, REG_A, REG_H},
    [0x95] = {SUB, REG_A, REG_L},
    [0x96] = {SUB, REG_A, DATA_HL},
    [0xD6] = {SUB, REG_A, IM_DATA8},
    // SBC
    [0x9F] = {SBC, REG_A, REG_A},
    [0x98] = {SBC, REG_A, REG_B},
    [0x99] = {SBC, REG_A, REG_C},
    [0x9A] = {SBC, REG_A, REG_D},
    [0x9B] = {SBC, REG_A, REG_E},
    [0x9C] = {SBC, REG_A, REG_H},
    [0x9D] = {SBC, REG_A, REG_L},
    [0x9E] = {SBC, REG_A, DATA_HL},
    [0xDE] = {SBC, REG_A, IM_DATA8},
    // AND
    [0xA7] = {AND, REG_A, REG_A},
    [0xA0] = {AND, REG_A, REG_B},
    [0xA1] = {AND, REG_A, REG_C},
    [0xA2] = {AND, REG_A, REG_D},
    [0xA3] = {AND, REG_A, REG_E},
    [0xA4] = {AND, REG_A, REG_H},
    [0xA5] = {AND, REG_A, REG_L},
    [0xA6] = {AND, REG_A, DATA_HL},
    [0xE6] = {AND, REG_A, IM_DATA8},
    // OR
    [0xB7] = {OR, REG_A, REG_A},
    [0xB0] = {OR, REG_A, REG_B},
    [0xB1] = {OR, REG_A, REG_C},
    [0xB2] = {OR, REG_A, REG_D},
    [0xB3] = {OR, REG_A, REG_E},
    [0xB4] = {OR, REG_A, REG_H},
    [0xB5] = {OR, REG_A, REG_L},
    [0xB6] = {OR, REG_A, DATA_HL},
    [0xF6] = {OR, REG_A, IM_DATA8},
    // XOR
    [0xAF] = {XOR, REG_A, REG_A},
    [0xA8] = {XOR, REG_A, REG_B},
    [0xA9] = {XOR, REG_A, REG_C},
    [0xAA] = {XOR, REG_A, REG_D},
    [0xAB] = {XOR, REG_A, REG_E},
    [0xAC] = {XOR, REG_A, REG_H},
    [0xAD] = {XOR, REG_A, REG_L},
    [0xAE] = {XOR, REG_A, DATA_HL},
    [0xEE] = {XOR, REG_A, IM_DATA8},
    // CP
    [0xBF] = {CP, REG_A, REG_A},
    [0xB8] = {CP, REG_A, REG_B},
    [0xB9] = {CP, REG_A, REG_C},
    [0xBA] = {CP, REG_A, REG_D},
    [0xBB] = {CP, REG_A, REG_E},
    [0xBC] = {CP, REG_A, REG_H},
    [0xBD] = {CP, REG_A, REG_L},
    [0xBE] = {CP, REG_A, DATA_HL},
    [0xFE] = {CP, REG_A, IM_DATA8},
    // INC
    [0x3C] = {INC, REG_A, NONE},
    [0x04] = {INC, REG_B, NONE},
    [0x0C] = {INC, REG_C, NONE},
    [0x14] = {INC, REG_D, NONE},
    [0x1C] = {INC, REG_E, NONE},
    [0x24] = {INC, REG_H, NONE},
    [0x2C] = {INC, REG_L, NONE},
    [0x34] = {INC, DATA_HL, NONE},
    // INC16
    [0x03] = {INC16, REG_BC, NONE},
    [0x13] = {INC16, REG_DE, NONE},
    [0x23] = {INC16, REG_HL, NONE},
    [0x33] = {INC16, REG_SP, NONE},
    // DEC
    [0x3D] = {DEC, REG_A, NONE},
    [0x05] = {DEC, REG_B, NONE},
    [0x0D] = {DEC, REG_C, NONE},
    [0x15] = {DEC, REG_D, NONE},
    [0x1D] = {DEC, REG_E, NONE},
    [0x25] = {DEC, REG_H, NONE},
    [0x2D] = {DEC, REG_L, NONE},
    [0x35] = {DEC, DATA_HL, NONE},
    // DEC16
    [0x0B] = {DEC16, REG_BC, NONE},
    [0x1B] = {DEC16, REG_DE, NONE},
    [0x2B] = {DEC16, REG_HL, NONE},
    [0x3B] = {DEC16, REG_SP, NONE},

    // DAA
    [0x27] = {DAA, NONE, NONE},
    // CPL
    [0x2F] = {CPL, NONE, NONE},
    // CCF
    [0x3F] = {CCF, NONE, NONE},
    // SCF
    [0x37] = {SCF, NONE, NONE},
    [0x00] = {NOP, NONE, NONE},
    [0x76] = {HALT, NONE, NONE},
    [0x10] = {STOP, NONE, NONE},
    [0xF3] = {DI, NONE, NONE},
    [0xFB] = {EI, NONE, NONE},
    // Rotates
    [0x07] = {RLCA, REG_A, NONE},
    [0x17] = {RLA, REG_A, NONE},
    [0x0F] = {RRCA, REG_A, NONE},
    [0x1F] = {RRA, REG_A, NONE},
    // JUMP
    // JP
    [0xC3] = {JP, NONE, IM_DATA16},
    [0xC2] = {JP, JP_NZ, IM_DATA16},
    [0xCA] = {JP, JP_Z, IM_DATA16},
    [0xD2] = {JP, JP_NC, IM_DATA16},
    [0xDA] = {JP, JP_C, IM_DATA16},
    [0xE9] = {JP, JP_HL, REG_HL},
    // JR - relative JUMP
    [0x18] = {JR, NONE, IM_DATA8},
    [0x20] = {JR, JP_NZ, IM_DATA8},
    [0x28] = {JR, JP_Z, IM_DATA8},
    [0x30] = {JR, JP_NC, IM_DATA8},
    [0x38] = {JR, JP_C, IM_DATA8},

    // CALL
    [0xCD] = {CALL, NONE, IM_DATA16},
    [0xC4] = {CALL, JP_NZ, IM_DATA16},
    [0xCC] = {CALL, JP_Z, IM_DATA16},
    [0xD4] = {CALL, JP_NC, IM_DATA16},
    [0xDC] = {CALL, JP_C, IM_DATA16},

    // RST
    // operand is the jump value
    [0xC7] = {RST, 0x00, NONE},
    [0xCF] = {RST, 0x08, NONE},
    [0xD7] = {RST, 0x10, NONE},
    [0xDF] = {RST, 0x18, NONE},
    [0xE7] = {RST, 0x20, NONE},
    [0xEF] = {RST, 0x28, NONE},
    [0xF7] = {RST, 0x30, NONE},
    [0xFF] = {RST, 0x38, NONE},

    // RET
    [0xC9] = {RET, NONE, NONE},
    [0xC0] = {RET, JP_NZ, NONE},
    [0xC8] = {RET, JP_Z, NONE},
    [0xD0] = {RET, JP_NC, NONE},
    [0xD8] = {RET, JP_C, NONE},
    [0xD9] = {RETI, NONE, NONE},

    [0xCB] = {CB, NONE, NONE},
};

instruction CB_instructions[256] = {
    // SWAP
    [0x37] = {SWAP, REG_A, NONE},
    [0x30] = {SWAP, REG_B, NONE},
    [0x31] = {SWAP, REG_C, NONE},
    [0x32] = {SWAP, REG_D, NONE},
    [0x33] = {SWAP, REG_E, NONE},
    [0x34] = {SWAP, REG_H, NONE},
    [0x35] = {SWAP, REG_L, NONE},
    [0x36] = {SWAP, DATA_HL, NONE},
    // RLC
    [0x07] = {RLC, REG_A, NONE},
    [0x00] = {RLC, REG_B, NONE},
    [0x01] = {RLC, REG_C, NONE},
    [0x02] = {RLC, REG_D, NONE},
    [0x03] = {RLC, REG_E, NONE},
    [0x04] = {RLC, REG_H, NONE},
    [0x05] = {RLC, REG_L, NONE},
    [0x06] = {RLC, DATA_HL, NONE},
    // RL
    [0x17] = {RL, REG_A, NONE},
    [0x10] = {RL, REG_B, NONE},
    [0x11] = {RL, REG_C, NONE},
    [0x12] = {RL, REG_D, NONE},
    [0x13] = {RL, REG_E, NONE},
    [0x14] = {RL, REG_H, NONE},
    [0x15] = {RL, REG_L, NONE},
    [0x16] = {RL, DATA_HL, NONE},
    // RRC
    [0x0F] = {RRC, REG_A, NONE},
    [0x08] = {RRC, REG_B, NONE},
    [0x09] = {RRC, REG_C, NONE},
    [0x0A] = {RRC, REG_D, NONE},
    [0x0B] = {RRC, REG_E, NONE},
    [0x0C] = {RRC, REG_H, NONE},
    [0x0D] = {RRC, REG_L, NONE},
    [0x0E] = {RRC, DATA_HL, NONE},
    // RR
    [0x1F] = {RR, REG_A, NONE},
    [0x18] = {RR, REG_B, NONE},
    [0x19] = {RR, REG_C, NONE},
    [0x1A] = {RR, REG_D, NONE},
    [0x1B] = {RR, REG_E, NONE},
    [0x1C] = {RR, REG_H, NONE},
    [0x1D] = {RR, REG_L, NONE},
    [0x1E] = {RR, DATA_HL, NONE},
    // SLA
    [0x27] = {SLA, REG_A, NONE},
    [0x20] = {SLA, REG_B, NONE},
    [0x21] = {SLA, REG_C, NONE},
    [0x22] = {SLA, REG_D, NONE},
    [0x23] = {SLA, REG_E, NONE},
    [0x24] = {SLA, REG_H, NONE},
    [0x25] = {SLA, REG_L, NONE},
    [0x26] = {SLA, DATA_HL, NONE},
    // SRA
    [0x2F] = {SRA, REG_A, NONE},
    [0x28] = {SRA, REG_B, NONE},
    [0x29] = {SRA, REG_C, NONE},
    [0x2A] = {SRA, REG_D, NONE},
    [0x2B] = {SRA, REG_E, NONE},
    [0x2C] = {SRA, REG_H, NONE},
    [0x2D] = {SRA, REG_L, NONE},
    [0x2E] = {SRA, DATA_HL, NONE},
    // SRL
    [0x3F] = {SRL, REG_A, NONE},
    [0x38] = {SRL, REG_B, NONE},
    [0x39] = {SRL, REG_C, NONE},
    [0x3A] = {SRL, REG_D, NONE},
    [0x3B] = {SRL, REG_E, NONE},
    [0x3C] = {SRL, REG_H, NONE},
    [0x3D] = {SRL, REG_L, NONE},
    [0x3E] = {SRL, DATA_HL, NONE},
    // BIT
    // instr op_a is the index of the bit
    // BIT_0
    [0x40] = {BIT, 0, REG_B},
    [0x41] = {BIT, 0, REG_C},
    [0x42] = {BIT, 0, REG_D},
    [0x43] = {BIT, 0, REG_E},
    [0x44] = {BIT, 0, REG_H},
    [0x45] = {BIT, 0, REG_L},
    [0x46] = {BIT, 0, DATA_HL},
    [0x47] = {BIT, 0, REG_A},
    // BIT_1
    [0x48] = {BIT, 1, REG_B},
    [0x49] = {BIT, 1, REG_C},
    [0x4A] = {BIT, 1, REG_D},
    [0x4B] = {BIT, 1, REG_E},
    [0x4C] = {BIT, 1, REG_H},
    [0x4D] = {BIT, 1, REG_L},
    [0x4E] = {BIT, 1, DATA_HL},
    [0x4F] = {BIT, 1, REG_A},
    // BIT_2
    [0x50] = {BIT, 2, REG_B},
    [0x51] = {BIT, 2, REG_C},
    [0x52] = {BIT, 2, REG_D},
    [0x53] = {BIT, 2, REG_E},
    [0x54] = {BIT, 2, REG_H},
    [0x55] = {BIT, 2, REG_L},
    [0x56] = {BIT, 2, DATA_HL},
    [0x57] = {BIT, 2, REG_A},
    // BIT_3
    [0x58] = {BIT, 3, REG_B},
    [0x59] = {BIT, 3, REG_C},
    [0x5A] = {BIT, 3, REG_D},
    [0x5B] = {BIT, 3, REG_E},
    [0x5C] = {BIT, 3, REG_H},
    [0x5D] = {BIT, 3, REG_L},
    [0x5E] = {BIT, 3, DATA_HL},
    [0x5F] = {BIT, 3, REG_A},
    // BIT_4
    [0x60] = {BIT, 4, REG_B},
    [0x61] = {BIT, 4, REG_C},
    [0x62] = {BIT, 4, REG_D},
    [0x63] = {BIT, 4, REG_E},
    [0x64] = {BIT, 4, REG_H},
    [0x65] = {BIT, 4, REG_L},
    [0x66] = {BIT, 4, DATA_HL},
    [0x67] = {BIT, 4, REG_A},
    // BIT_5
    [0x68] = {BIT, 5, REG_B},
    [0x69] = {BIT, 5, REG_C},
    [0x6A] = {BIT, 5, REG_D},
    [0x6B] = {BIT, 5, REG_E},
    [0x6C] = {BIT, 5, REG_H},
    [0x6D] = {BIT, 5, REG_L},
    [0x6E] = {BIT, 5, DATA_HL},
    [0x6F] = {BIT, 5, REG_A},
    // BIT_6
    [0x70] = {BIT, 6, REG_B},
    [0x71] = {BIT, 6, REG_C},
    [0x72] = {BIT, 6, REG_D},
    [0x73] = {BIT, 6, REG_E},
    [0x74] = {BIT, 6, REG_H},
    [0x75] = {BIT, 6, REG_L},
    [0x76] = {BIT, 6, DATA_HL},
    [0x77] = {BIT, 6, REG_A},
    // BIT_7
    [0x78] = {BIT, 7, REG_B},
    [0x79] = {BIT, 7, REG_C},
    [0x7A] = {BIT, 7, REG_D},
    [0x7B] = {BIT, 7, REG_E},
    [0x7C] = {BIT, 7, REG_H},
    [0x7D] = {BIT, 7, REG_L},
    [0x7E] = {BIT, 7, DATA_HL},
    [0x7F] = {BIT, 7, REG_A},
    // SET
    // BIT_0
    [0xC0] = {SET, 0, REG_B},
    [0xC1] = {SET, 0, REG_C},
    [0xC2] = {SET, 0, REG_D},
    [0xC3] = {SET, 0, REG_E},
    [0xC4] = {SET, 0, REG_H},
    [0xC5] = {SET, 0, REG_L},
    [0xC6] = {SET, 0, DATA_HL},
    [0xC7] = {SET, 0, REG_A},
    // BIT_1
    [0xC8] = {SET, 1, REG_B},
    [0xC9] = {SET, 1, REG_C},
    [0xCA] = {SET, 1, REG_D},
    [0xCB] = {SET, 1, REG_E},
    [0xCC] = {SET, 1, REG_H},
    [0xCD] = {SET, 1, REG_L},
    [0xCE] = {SET, 1, DATA_HL},
    [0xCF] = {SET, 1, REG_A},
    // BIT_2
    [0xD0] = {SET, 2, REG_B},
    [0xD1] = {SET, 2, REG_C},
    [0xD2] = {SET, 2, REG_D},
    [0xD3] = {SET, 2, REG_E},
    [0xD4] = {SET, 2, REG_H},
    [0xD5] = {SET, 2, REG_L},
    [0xD6] = {SET, 2, DATA_HL},
    [0xD7] = {SET, 2, REG_A},
    // BIT_3
    [0xD8] = {SET, 3, REG_B},
    [0xD9] = {SET, 3, REG_C},
    [0xDA] = {SET, 3, REG_D},
    [0xDB] = {SET, 3, REG_E},
    [0xDC] = {SET, 3, REG_H},
    [0xDD] = {SET, 3, REG_L},
    [0xDE] = {SET, 3, DATA_HL},
    [0xDF] = {SET, 3, REG_A},
    // BIT_4
    [0xE0] = {SET, 4, REG_B},
    [0xE1] = {SET, 4, REG_C},
    [0xE2] = {SET, 4, REG_D},
    [0xE3] = {SET, 4, REG_E},
    [0xE4] = {SET, 4, REG_H},
    [0xE5] = {SET, 4, REG_L},
    [0xE6] = {SET, 4, DATA_HL},
    [0xE7] = {SET, 4, REG_A},
    // BIT_5
    [0xE8] = {SET, 5, REG_B},
    [0xE9] = {SET, 5, REG_C},
    [0xEA] = {SET, 5, REG_D},
    [0xEB] = {SET, 5, REG_E},
    [0xEC] = {SET, 5, REG_H},
    [0xED] = {SET, 5, REG_L},
    [0xEE] = {SET, 5, DATA_HL},
    [0xEF] = {SET, 5, REG_A},
    // BIT_6
    [0xF0] = {SET, 6, REG_B},
    [0xF1] = {SET, 6, REG_C},
    [0xF2] = {SET, 6, REG_D},
    [0xF3] = {SET, 6, REG_E},
    [0xF4] = {SET, 6, REG_H},
    [0xF5] = {SET, 6, REG_L},
    [0xF6] = {SET, 6, DATA_HL},
    [0xF7] = {SET, 6, REG_A},
    // BIT_7
    [0xF8] = {SET, 7, REG_B},
    [0xF9] = {SET, 7, REG_C},
    [0xFA] = {SET, 7, REG_D},
    [0xFB] = {SET, 7, REG_E},
    [0xFC] = {SET, 7, REG_H},
    [0xFD] = {SET, 7, REG_L},
    [0xFE] = {SET, 7, DATA_HL},
    [0xFF] = {SET, 7, REG_A},
    // RES
    // BIT_0
    [0x80] = {RES, 0, REG_B},
    [0x81] = {RES, 0, REG_C},
    [0x82] = {RES, 0, REG_D},
    [0x83] = {RES, 0, REG_E},
    [0x84] = {RES, 0, REG_H},
    [0x85] = {RES, 0, REG_L},
    [0x86] = {RES, 0, DATA_HL},
    [0x87] = {RES, 0, REG_A},
    // BIT_1
    [0x88] = {RES, 1, REG_B},
    [0x89] = {RES, 1, REG_C},
    [0x8A] = {RES, 1, REG_D},
    [0x8B] = {RES, 1, REG_E},
    [0x8C] = {RES, 1, REG_H},
    [0x8D] = {RES, 1, REG_L},
    [0x8E] = {RES, 1, DATA_HL},
    [0x8F] = {RES, 1, REG_A},
    // BIT_2
    [0x90] = {RES, 2, REG_B},
    [0x91] = {RES, 2, REG_C},
    [0x92] = {RES, 2, REG_D},
    [0x93] = {RES, 2, REG_E},
    [0x94] = {RES, 2, REG_H},
    [0x95] = {RES, 2, REG_L},
    [0x96] = {RES, 2, DATA_HL},
    [0x97] = {RES, 2, REG_A},
    // BIT_3
    [0x98] = {RES, 3, REG_B},
    [0x99] = {RES, 3, REG_C},
    [0x9A] = {RES, 3, REG_D},
    [0x9B] = {RES, 3, REG_E},
    [0x9C] = {RES, 3, REG_H},
    [0x9D] = {RES, 3, REG_L},
    [0x9E] = {RES, 3, DATA_HL},
    [0x9F] = {RES, 3, REG_A},
    // BIT_4
    [0xA0] = {RES, 4, REG_B},
    [0xA1] = {RES, 4, REG_C},
    [0xA2] = {RES, 4, REG_D},
    [0xA3] = {RES, 4, REG_E},
    [0xA4] = {RES, 4, REG_H},
    [0xA5] = {RES, 4, REG_L},
    [0xA6] = {RES, 4, DATA_HL},
    [0xA7] = {RES, 4, REG_A},
    // BIT_5
    [0xA8] = {RES, 5, REG_B},
    [0xA9] = {RES, 5, REG_C},
    [0xAA] = {RES, 5, REG_D},
    [0xAB] = {RES, 5, REG_E},
    [0xAC] = {RES, 5, REG_H},
    [0xAD] = {RES, 5, REG_L},
    [0xAE] = {RES, 5, DATA_HL},
    [0xAF] = {RES, 5, REG_A},
    // BIT_6
    [0xB0] = {RES, 6, REG_B},
    [0xB1] = {RES, 6, REG_C},
    [0xB2] = {RES, 6, REG_D},
    [0xB3] = {RES, 6, REG_E},
    [0xB4] = {RES, 6, REG_H},
    [0xB5] = {RES, 6, REG_L},
    [0xB6] = {RES, 6, DATA_HL},
    [0xB7] = {RES, 6, REG_A},
    // BIT_7
    [0xB8] = {RES, 7, REG_B},
    [0xB9] = {RES, 7, REG_C},
    [0xBA] = {RES, 7, REG_D},
    [0xBB] = {RES, 7, REG_E},
    [0xBC] = {RES, 7, REG_H},
    [0xBD] = {RES, 7, REG_L},
    [0xBE] = {RES, 7, DATA_HL},
    [0xBF] = {RES, 7, REG_A},
};

instruction opcode_to_instr(u8 op_code, bool isCB) { return (!isCB) ? instructions[op_code] : CB_instructions[op_code]; };
