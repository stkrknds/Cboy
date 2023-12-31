#include "cpu.h"
#include "joypad.h"
#include "ppu.h"
#include "timers.h"
#include "timing.h"

#include <stdbool.h>

static cpu _cpu;
u8 IE_register;
u8 IF_register;

#ifdef TEST_CHECK
bool testPassed = false;
#endif

static u8 reg_read(cpu cpu, instr_op reg) {
    switch (reg) {
        case REG_A:
            return cpu.A;
        case REG_B:
            return cpu.B;
        case REG_C:
            return cpu.C;
        case REG_D:
            return cpu.D;
        case REG_E:
            return cpu.E;
        case REG_H:
            return cpu.H;
        case REG_L:
            return cpu.L;
        default:
            printf("Attempted to read an invalid register. \n");
            exit(0);
    }
};

static u16 reg_read16(cpu cpu, instr_op reg) {
    switch (reg) {
        case REG_HL:
            return *((u16 *)&cpu.L);
        case REG_BC:
            return *((u16 *)&cpu.C);
        case REG_DE:
            return *((u16 *)&cpu.E);
        case REG_AF:
            return *((u16 *)&cpu.F);
        case REG_SP:
            return cpu.SP;
        case REG_PC:
            return cpu.PC;
        default:
            printf("Attempted to read an invalid register. \n");
            exit(0);
    }
};

static void reg_write(cpu *cpu, instr_op reg, u8 val) {
    switch (reg) {
        case REG_A:
            cpu->A = val;
            break;
        case REG_B:
            cpu->B = val;
            break;
        case REG_C:
            cpu->C = val;
            break;
        case REG_D:
            cpu->D = val;
            break;
        case REG_E:
            cpu->E = val;
            break;
        case REG_H:
            cpu->H = val;
            break;
        case REG_L:
            cpu->L = val;
            break;
        default:
            printf("Attempted to write to a non valid register. \n");
            exit(0);
    }
};

static void reg_write16(cpu *cpu, instr_op reg, u16 val) {
    switch (reg) {
        case REG_HL:
            *((u16 *)(&cpu->L)) = val;
            break;
        case REG_BC:
            *((u16 *)(&cpu->C)) = val;
            break;
        case REG_DE:
            *((u16 *)(&cpu->E)) = val;
            break;
        case REG_AF:
            // 4 lsbs are always 0 in register F
            *((u16 *)(&cpu->F)) = val & 0xFFF0;
            break;
        case REG_SP:
            cpu->SP = val;
            return;
        case REG_PC:
            cpu->PC = val;
            return;
        default:
            printf("Attempted to write to a non valid register. \n");
            exit(0);
    }
};

#ifdef DEBUG
static void reg_print(FILE *fp, cpu cpu) {
    fprintf(fp, "A:%02X ", cpu.A);
    fprintf(fp, "F:%02X ", cpu.F);
    fprintf(fp, "B:%02X ", cpu.B);
    fprintf(fp, "C:%02X ", cpu.C);
    fprintf(fp, "D:%02X ", cpu.D);
    fprintf(fp, "E:%02X ", cpu.E);
    fprintf(fp, "H:%02X ", cpu.H);
    fprintf(fp, "L:%02X ", cpu.L);
    fprintf(fp, "SP:%04X ", cpu.SP);
    fprintf(fp, "PC:%04X ", cpu.PC);
    fprintf(fp, "TIMA:%02X ", tima.reg);
    fprintf(fp, "DIV:%04X ", DIV_register);
    fprintf(fp, "PCMEM:%02X,%02X,%02X,%02X", bus_read(cpu.PC, false), bus_read(cpu.PC + 1, false), bus_read(cpu.PC + 2, false), bus_read(cpu.PC + 3, false));
    fprintf(fp, "\n");
};
#endif

// TODO remove fetch_data completely
static u16 fetch_data(cpu *cpu, instr_op addr) {
    u16 data;

    switch (addr) {
        case REG_A:
        case REG_B:
        case REG_C:
        case REG_D:
        case REG_E:
        case REG_H:
        case REG_L:
            data = reg_read(*cpu, addr);
            break;
        case REG_BC:
        case REG_DE:
        case REG_HL:
        case REG_SP:
        case REG_PC:
        case REG_AF:
            data = reg_read16(*cpu, addr);
            break;
        case DATA_HL: {
            u16 raddr = reg_read16(*cpu, REG_HL);
            data = bus_read(raddr, true);
            break;
        }
        case DATA_BC: {
            u16 raddr = reg_read16(*cpu, REG_BC);
            data = bus_read(raddr, true);
            break;
        }
        case DATA_DE: {
            u16 raddr = reg_read16(*cpu, REG_DE);
            data = bus_read(raddr, true);
            break;
        }
        case DATA_N: {
            // get 1 byte and add it to 0xFF00
            u8 val = bus_read(cpu->PC++, true);
            u16 raddr = 0xFF00 + val;
            data = bus_read(raddr, true);
            break;
        }
        case DATA_C: {
            u8 val = reg_read(*cpu, REG_C);
            addr = 0xFF00 + val;
            data = bus_read(addr, true);
            break;
        }
        case IM_DATA8:
            data = bus_read(cpu->PC++, true);
            break;
        case IM_DATA16: {
            val16 val;
            val.lsb = bus_read(cpu->PC++, true);
            val.msb = bus_read(cpu->PC++, true);
            data = val.val;
            break;
        }
        case DATA_NN: {
            u16 raddr = fetch_data(cpu, IM_DATA16);
            data = bus_read(raddr, true);
            break;
        }
        default:
            printf("Invalid addr %d \n", addr);
            exit(0);
    }
    return data;
};

// TODO remove write_data completely
static void write_data(cpu *cpu, instr_op instr_op, u16 val) {
    switch (instr_op) {
        case REG_A:
        case REG_B:
        case REG_C:
        case REG_D:
        case REG_E:
        case REG_H:
        case REG_L:
            reg_write(cpu, instr_op, val);
            break;
        case REG_AF:
        case REG_BC:
        case REG_DE:
        case REG_HL:
        case REG_SP:
        case REG_PC:
            reg_write16(cpu, instr_op, val);
            break;
        case DATA_HL: {
            u16 wraddr = reg_read16(*cpu, REG_HL);
            bus_write(wraddr, val, true);
            break;
        }
        case DATA_BC: {
            u16 wraddr = reg_read16(*cpu, REG_BC);
            bus_write(wraddr, val, true);
            break;
        }
        case DATA_DE: {
            u16 wraddr = reg_read16(*cpu, REG_DE);
            bus_write(wraddr, val, true);
            break;
        }
        case DATA_NN: {
            u16 wraddr = fetch_data(cpu, IM_DATA16);
            bus_write(wraddr, val, true);
            break;
        }
        case DATA_NN16: {
            u16 wraddr = fetch_data(cpu, IM_DATA16);
            bus_write(wraddr++, u16_lsb(&val), true);
            bus_write(wraddr, u16_msb(&val), true);
            break;
        }
        case DATA_N: {
            u16 wraddr = fetch_data(cpu, IM_DATA8);
            wraddr += 0xFF00;
            bus_write(wraddr, val, true);
            break;
        }
        case DATA_C: {
            u16 wraddr = reg_read(*cpu, REG_C);
            wraddr += 0xFF00;
            bus_write(wraddr, val, true);
            break;
        }
        default:
            printf("Invalid addr %d", instr_op);
            exit(0);
    }
};

static bool flag_reg_read(cpu *cpu, FLAG flag) { return bit_read(cpu->F, flag); }

static void flag_reg_write(cpu *cpu, FLAG flag, bool bval) { bit_write(&cpu->F, flag, bval); }

static bool add_isHC8(u8 a, u8 b) { return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10; }

static bool add_isC8(u8 a, u8 b) { return ((a + b) & 0x100) == 0x100; }

static bool add_isHC16(u16 a, u16 b) { return (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000; }

static bool add_isC16(u16 a, u16 b) { return ((a + b) & 0x10000) == 0x10000; }

static bool adc_isHC8(u8 a, u8 b, u8 c) { return (((a & 0x0F) + (b & 0x0F) + (c & 0x0F)) & 0x10) == 0x10; }

static bool adc_isC8(u8 a, u8 b, u8 c) { return ((a + b + c) & 0x100) == 0x100; }

static bool sub_isHC8(u8 a, u8 b) { return (((a & 0xF) - (b & 0xF)) & 0x10) == 0x10; }

static bool sub_isC8(u8 a, u8 b) { return ((a - b) & 0x100) == 0x100; }

static bool addSigned_isHC(u16 a, int8 b) { return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10; }

static bool addSigned_isC(u16 a, int8 b) { return (((a & 0xFF) + (b & 0xFF)) & 0x100) == 0x100; }

static void stack_push(cpu *cpu, instr_op reg) {
    tick_MCycle();
    tick_MCycle();
    val16 val = (val16)reg_read16(*cpu, reg);
    bus_write(--cpu->SP, val.msb, true);
    bus_write(--cpu->SP, val.lsb, true);
}

static void stack_pop(cpu *cpu, instr_op reg) {
    val16 val;
    val.lsb = bus_read(cpu->SP++, true);
    val.msb = bus_read(cpu->SP++, true);
    reg_write16(cpu, reg, val.val);
}

static void clear_NH_flags(cpu *cpu) {
    bit_clear(&cpu->F, N);
    bit_clear(&cpu->F, H);
}

static void execute_SWAP(cpu *cpu, instruction instr) {
    u8 currVal;
    u8 swappedVal;

    tick_MCycle();
    currVal = fetch_data(cpu, instr.op_a);
    swappedVal = (currVal >> 4) | (currVal << 4);

    write_data(cpu, instr.op_a, swappedVal);

    flag_reg_write(cpu, Z, swappedVal == 0);
    clear_NH_flags(cpu);
    bit_clear(&cpu->F, C);
    tick_MCycle();
}

// taken from https://ehaskins.com/2018-01-30%20Z80%20DAA/
static void execute_DAA(cpu *cpu) {
    u8 val = cpu->A;

    if (!flag_reg_read(cpu, N)) { // after an addition, adjust if (half-)carry occurred or if result is out of bounds
        if (flag_reg_read(cpu, C) || val > 0x99) {
            val += 0x60;
            flag_reg_write(cpu, C, 1);
        }
        if (flag_reg_read(cpu, H) || (val & 0x0f) > 0x09) {
            val += 0x6;
        }
    }
    else { // after a subtraction, only adjust if (half-)carry occurred
        if (flag_reg_read(cpu, C)) {
            val -= 0x60;
        }
        if (flag_reg_read(cpu, H)) {
            val -= 0x6;
        }
    }

    flag_reg_write(cpu, Z, val == 0);
    bit_clear(&cpu->F, H);

    tick_MCycle();

    cpu->A = val;
}

static void execute_CPL(cpu *cpu) {
    cpu->A = ~cpu->A;

    bit_set(&cpu->F, N);
    bit_set(&cpu->F, H);

    tick_MCycle();
}

static void execute_CCF(cpu *cpu) {
    bool Cflag = flag_reg_read(cpu, C);

    flag_reg_write(cpu, C, !Cflag);
    clear_NH_flags(cpu);

    tick_MCycle();
}

void execute_SCF(cpu *cpu) {
    clear_NH_flags(cpu);
    bit_set(&cpu->F, C);

    tick_MCycle();
}

void execute_NOP() {
    // do nothing
    tick_MCycle();
}

void execute_HALT(cpu *cpu) {
    // check if there are any interrupts pending
    bool isIntrPending = (IE_register & IF_register & 0x1F) != 0;

    if (isIntrPending) {
        // if there are interrupts pending and the IME flag is enabled, the cpu isn't halted
        if (cpu->IME)
            cpu->isHalted = false;
        else {
            // if interrupts are pending and the IME flag is disabled, halt bug occurs
            cpu->isHaltBug = true;
            cpu->isHalted = false;
        }
    }
    else {
        // if there are no interrupts pending, the cpu is halted
        cpu->isHalted = true;
        tick_MCycle();
    }
}

void execute_STOP() {
    // TODO
    printf("Instruction STOP is not implemented. \n");
}

static void execute_DI(cpu *cpu) {
    cpu->IME = false;
    cpu->scheduledIME = false;
    tick_MCycle();
}

void execute_EI(cpu *cpu) {
    // the IME flag is enabled after one M cycle(= 4 T Cycles)
    cpu->scheduledIME = true;
    tick_MCycle();
}

static void execute_RL(cpu *cpu, instruction instr) {
    u8 val;
    u8 shiftedVal;
    u8 newVal;
    bool msb;
    bool Cflag;

    tick_MCycle();
    val = fetch_data(cpu, instr.op_a);
    // msb goes to the C flag
    msb = bit_read(val, 7);
    shiftedVal = val << 1;

    Cflag = flag_reg_read(cpu, C);
    newVal = shiftedVal + Cflag;

    write_data(cpu, instr.op_a, newVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, newVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, msb);
}

static void execute_RLC(cpu *cpu, instruction instr) {
    u8 val;
    u8 shiftedVal;
    bool msb;

    tick_MCycle();
    val = fetch_data(cpu, instr.op_a);
    msb = bit_read(val, 7);
    shiftedVal = ((val << 1) & 0xFE) | msb;

    write_data(cpu, instr.op_a, shiftedVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, shiftedVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, msb);
}

static void execute_RLCA(cpu *cpu) {
    u8 val = cpu->A;
    bool msb = bit_read(val, 7);

    cpu->A = ((val << 1) & 0xFE) | msb;

    bit_clear(&cpu->F, Z);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, msb);

    tick_MCycle();
}

static void execute_RLA(cpu *cpu) {
    u8 val = cpu->A;
    bool msb = bit_read(val, 7);
    bool Cflag = flag_reg_read(cpu, C);

    cpu->A = (val << 1) | Cflag;

    bit_clear(&cpu->F, Z);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, msb);

    tick_MCycle();
}

static void execute_RR(cpu *cpu, instruction instr) {
    u8 val;
    u8 shiftedVal;
    u8 newVal;
    bool lsb;
    bool Cflag;

    tick_MCycle();
    val = fetch_data(cpu, instr.op_a);

    lsb = bit_read(val, 0);
    shiftedVal = val >> 1;
    Cflag = flag_reg_read(cpu, C);

    newVal = shiftedVal | (Cflag << 7);

    write_data(cpu, instr.op_a, newVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, newVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
}

static void execute_RRC(cpu *cpu, instruction instr) {
    u8 val;
    bool lsb;
    u8 shiftedVal;
    u8 final_val;

    tick_MCycle();
    val = fetch_data(cpu, instr.op_a);
    lsb = bit_read(val, 0);
    shiftedVal = 0x7F & (val >> 1);

    final_val = shiftedVal | (lsb << 7);

    write_data(cpu, instr.op_a, final_val);
    tick_MCycle();

    flag_reg_write(cpu, Z, final_val == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
}

static void execute_RRCA(cpu *cpu) {
    u8 val = cpu->A;
    bool lsb = bit_read(val, 0);
    cpu->A = (0x7F & (val >> 1)) | (lsb << 7);

    bit_clear(&cpu->F, Z);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
    tick_MCycle();
}

static void execute_RRA(cpu *cpu) {
    u8 val = cpu->A;
    bool lsb = bit_read(val, 0);
    bool Cflag = flag_reg_read(cpu, C);

    cpu->A = (val >> 1) | (Cflag << 7);

    bit_clear(&cpu->F, Z);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
    tick_MCycle();
}

static void execute_SLA(cpu *cpu, instruction instr) {
    tick_MCycle();
    u8 val = fetch_data(cpu, instr.op_a);
    u8 msb = bit_read(val, 7);
    u8 shiftedVal = val << 1;

    write_data(cpu, instr.op_a, shiftedVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, shiftedVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, msb);
}

static void execute_SRA(cpu *cpu, instruction instr) {
    tick_MCycle();
    u8 val = fetch_data(cpu, instr.op_a);
    bool lsb = bit_read(val, 0);
    u8 shiftedVal = (val >> 1) | (val & 0x80);

    write_data(cpu, instr.op_a, shiftedVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, shiftedVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
}

static void execute_SRL(cpu *cpu, instruction instr) {
    tick_MCycle();
    u8 val = fetch_data(cpu, instr.op_a);
    u8 lsb = bit_read(val, 0);
    u8 shiftedVal = val >> 1;

    write_data(cpu, instr.op_a, shiftedVal);
    tick_MCycle();

    flag_reg_write(cpu, Z, shiftedVal == 0);
    clear_NH_flags(cpu);
    flag_reg_write(cpu, C, lsb);
}

static void execute_ADD(cpu *cpu, instruction instr) {
    u8 regA = cpu->A;
    u8 b = fetch_data(cpu, instr.op_b);

    u8 res = regA + b;

    flag_reg_write(cpu, Z, res == 0);
    bit_clear(&cpu->F, N);
    flag_reg_write(cpu, H, add_isHC8(regA, b));
    flag_reg_write(cpu, C, add_isC8(regA, b));

    reg_write(cpu, REG_A, res);

    tick_MCycle();
}

static void execute_ADDSP(cpu *cpu) {
    // TODO fix timing
    tick_MCycle();

    u16 SPval = cpu->SP;
    u8 val = fetch_data(cpu, IM_DATA8);
    int8 e = (int8)val;
    u16 res = SPval + e;

    tick_MCycle();

    bit_clear(&cpu->F, Z);
    bit_clear(&cpu->F, N);
    flag_reg_write(cpu, H, addSigned_isHC(SPval, e));
    flag_reg_write(cpu, C, addSigned_isC(SPval, e));

    reg_write16(cpu, REG_SP, res);

    tick_MCycle();
}

static void execute_ADD16(cpu *cpu, instruction instr) {
    u16 a = fetch_data(cpu, instr.op_a);
    u16 b = fetch_data(cpu, instr.op_b);
    u16 res = a + b;

    tick_MCycle();

    bit_clear(&cpu->F, N);
    flag_reg_write(cpu, H, add_isHC16(a, b));
    flag_reg_write(cpu, C, add_isC16(a, b));

    reg_write16(cpu, REG_HL, res);

    tick_MCycle();
}

static void execute_ADC(cpu *cpu, instruction instr) {
    u8 a = fetch_data(cpu, instr.op_a);
    u8 b = fetch_data(cpu, instr.op_b);
    bool C_flag = flag_reg_read(cpu, C);
    u8 sum = a + b + C_flag;

    cpu->A = sum;

    flag_reg_write(cpu, Z, sum == 0);
    bit_clear(&cpu->F, N);
    flag_reg_write(cpu, H, adc_isHC8(a, b, C_flag));
    flag_reg_write(cpu, C, adc_isC8(a, b, C_flag));

    tick_MCycle();
}

static void execute_SUB(cpu *cpu, instruction instr) {
    u8 b = fetch_data(cpu, instr.op_b);
    u8 a = cpu->A;
    u8 res = a - b;

    cpu->A = res;

    flag_reg_write(cpu, Z, res == 0);
    bit_set(&cpu->F, N);
    flag_reg_write(cpu, H, sub_isHC8(a, b));
    flag_reg_write(cpu, C, sub_isC8(a, b));

    tick_MCycle();
}

static void execute_SBC(cpu *cpu, instruction instr) {
    u8 b = fetch_data(cpu, instr.op_b);
    u8 a = cpu->A;
    u16 res = a - (b + flag_reg_read(cpu, C));

    cpu->A = (u8)res;

    u8 HCflag = (((a & 0xF) - ((b & 0xF) + flag_reg_read(cpu, C))) & 0x10) == 0x10;
    u8 Cflag = res > 0xFF;

    flag_reg_write(cpu, Z, (u8)res == 0);
    bit_set(&cpu->F, N);
    flag_reg_write(cpu, H, HCflag);
    flag_reg_write(cpu, C, Cflag);

    tick_MCycle();
}

static void execute_OR(cpu *cpu, instruction instr) {
    u8 data = fetch_data(cpu, instr.op_b);

    cpu->A |= data;

    flag_reg_write(cpu, Z, cpu->A == 0);
    clear_NH_flags(cpu);
    bit_clear(&cpu->F, C);

    tick_MCycle();
}

static void execute_XOR(cpu *cpu, instruction instr) {
    u8 data = fetch_data(cpu, instr.op_b);

    cpu->A ^= data;

    flag_reg_write(cpu, Z, cpu->A == 0);
    clear_NH_flags(cpu);
    bit_clear(&cpu->F, C);

    tick_MCycle();
}

static void execute_AND(cpu *cpu, instruction instr) {
    u8 data = fetch_data(cpu, instr.op_b);

    cpu->A &= data;

    flag_reg_write(cpu, Z, cpu->A == 0);
    bit_clear(&cpu->F, N);
    bit_set(&cpu->F, H);
    bit_clear(&cpu->F, C);

    tick_MCycle();
}

static void execute_DEC(cpu *cpu, instruction instr) {
    u8 currVal = fetch_data(cpu, instr.op_a);
    u8 res = currVal - 1;

    write_data(cpu, instr.op_a, res);

    flag_reg_write(cpu, Z, res == 0);
    bit_set(&cpu->F, N);
    flag_reg_write(cpu, H, sub_isHC8(currVal, 1));

    tick_MCycle();
}

static void execute_DEC16(cpu *cpu, instruction instr) {
    tick_MCycle();

    u16 currVal = reg_read16(*cpu, instr.op_a);
    u16 res = currVal - 1;

    reg_write16(cpu, instr.op_a, res);

    tick_MCycle();
}

static void execute_INC(cpu *cpu, instruction instr) {
    u8 currVal = fetch_data(cpu, instr.op_a);
    u8 sum = currVal + 1;

    flag_reg_write(cpu, Z, sum == 0);
    bit_clear(&cpu->F, N);
    flag_reg_write(cpu, H, add_isHC8(currVal, 1));

    write_data(cpu, instr.op_a, sum);

    tick_MCycle();
}

static void execute_INC16(cpu *cpu, instruction instr) {
    u16 currVal = reg_read16(*cpu, instr.op_a);
    u16 sum = currVal + 1;

    tick_MCycle();

    reg_write16(cpu, instr.op_a, sum);

    tick_MCycle();
}

static void execute_CP(cpu *cpu, instruction instr) {
    u8 regA = cpu->A;
    u8 data = fetch_data(cpu, instr.op_b);
    u8 res = regA - data;

    flag_reg_write(cpu, Z, res == 0);
    bit_set(&cpu->F, N);
    flag_reg_write(cpu, H, sub_isHC8(regA, data));
    flag_reg_write(cpu, C, sub_isC8(regA, data));

    tick_MCycle();
}

static void execute_RES(cpu *cpu, instruction instr) {
    tick_MCycle();

    u8 val = fetch_data(cpu, instr.op_b);
    u8 idx = instr.op_a;

    bit_clear(&val, idx);
    write_data(cpu, instr.op_b, val);
    tick_MCycle();
}

static void execute_SET(cpu *cpu, instruction instr) {
    tick_MCycle();

    u8 val = fetch_data(cpu, instr.op_b);
    u8 idx = instr.op_a;

    bit_set(&val, idx);
    write_data(cpu, instr.op_b, (u16)val);

    tick_MCycle();
}

static void execute_BIT(cpu *cpu, instruction instr) {
    tick_MCycle();

    u8 val = fetch_data(cpu, instr.op_b);
    u8 idx = instr.op_a;

    tick_MCycle();

    flag_reg_write(cpu, Z, !bit_read(val, idx));
    bit_clear(&cpu->F, N);
    bit_set(&cpu->F, H);
}

// TODO refactor
static u16 execute_LD(cpu *cpu, instruction instr) {
    u16 data;
    u8 u8_data;
    int8 int8_data;
    switch (instr.type) {
        case LD:
            data = fetch_data(cpu, instr.op_b);
            write_data(cpu, instr.op_a, data);

            tick_MCycle();
            break;
        case LD_SP_HL:
            tick_MCycle();

            data = fetch_data(cpu, instr.op_b);
            write_data(cpu, instr.op_a, data);

            tick_MCycle();
            break;
        case LDD:
            data = fetch_data(cpu, instr.op_b);
            write_data(cpu, instr.op_a, data);
            (*(u16 *)(&cpu->L))--;

            tick_MCycle();
            break;
        case LDI:
            data = fetch_data(cpu, instr.op_b);
            write_data(cpu, instr.op_a, data);
            (*(u16 *)(&cpu->L))++;

            tick_MCycle();
            break;
        case LDHL: {
            tick_MCycle();
            data = fetch_data(cpu, IM_DATA8);
            u8_data = (u8)data;
            int8_data = (int8)u8_data;

            u16 val_SP = cpu->SP;
            u16 fdata = int8_data + val_SP;
            write_data(cpu, REG_HL, fdata);

            bit_clear(&cpu->F, Z);
            bit_clear(&cpu->F, N);
            flag_reg_write(cpu, H, addSigned_isHC(val_SP, int8_data));
            flag_reg_write(cpu, C, addSigned_isC(val_SP, int8_data));

            tick_MCycle();
            break;
        }
        default:
            printf("Invalid LD type %d \n", instr.type);
            exit(9);
    }
    return 0;
};

static void execute_JP(cpu *cpu, instruction instr) {
    u16 nn = fetch_data(cpu, instr.op_b);
    switch (instr.op_a) {
        case NONE:
            tick_MCycle();
            reg_write16(cpu, REG_PC, nn);
            tick_MCycle();
            break;
        case JP_HL:
            tick_MCycle();
            reg_write16(cpu, REG_PC, nn);
            break;
        case JP_NZ:
            tick_MCycle();
            if (!flag_reg_read(cpu, Z)) {
                reg_write16(cpu, REG_PC, nn);
                tick_MCycle();
            }
            break;
        case JP_Z:
            tick_MCycle();
            if (flag_reg_read(cpu, Z)) {
                reg_write16(cpu, REG_PC, nn);
                tick_MCycle();
            }
            break;
        case JP_NC:
            tick_MCycle();
            if (!flag_reg_read(cpu, C)) {
                reg_write16(cpu, REG_PC, nn);
                tick_MCycle();
            }
            break;
        case JP_C:
            tick_MCycle();
            if (flag_reg_read(cpu, C)) {
                reg_write16(cpu, REG_PC, nn);
                tick_MCycle();
            }
            break;
        default:
            printf("Invalid JP operator %d \n", instr.op_a);
            exit(9);
    }
}

static void execute_JR(cpu *cpu, instruction instr) {
    int8 data = (int8)bus_read(cpu->PC++, true);
    u16 curr_addr = reg_read16(*cpu, REG_PC);
    tick_MCycle();

    switch (instr.op_a) {
        case NONE:
            reg_write16(cpu, REG_PC, curr_addr + data);
            tick_MCycle();
            break;
        case JP_NZ:
            if (!flag_reg_read(cpu, Z)) {
                reg_write16(cpu, REG_PC, curr_addr + data);
                tick_MCycle();
            }
            break;
        case JP_Z:
            if (flag_reg_read(cpu, Z)) {
                reg_write16(cpu, REG_PC, curr_addr + data);
                tick_MCycle();
            }
            break;
        case JP_NC:
            if (!flag_reg_read(cpu, C)) {
                reg_write16(cpu, REG_PC, curr_addr + data);
                tick_MCycle();
            }
            break;
        case JP_C:
            if (flag_reg_read(cpu, C)) {
                reg_write16(cpu, REG_PC, curr_addr + data);
                tick_MCycle();
            }
            break;
        default:
            printf("Non valid JR instruction. \n");
            break;
    }
}

static void execute_CALL(cpu *cpu, instruction instr) {
    u16 next_addr = fetch_data(cpu, IM_DATA16);

    switch (instr.op_a) {
        case NONE:
            stack_push(cpu, REG_PC);
            reg_write16(cpu, REG_PC, next_addr);
            break;
        case JP_NZ:
            if (!flag_reg_read(cpu, Z)) {
                stack_push(cpu, REG_PC);
                reg_write16(cpu, REG_PC, next_addr);
            }
            else
                tick_MCycle();
            break;
        case JP_Z:
            if (flag_reg_read(cpu, Z)) {
                stack_push(cpu, REG_PC);
                reg_write16(cpu, REG_PC, next_addr);
            }
            else
                tick_MCycle();
            break;
        case JP_NC:
            if (!flag_reg_read(cpu, C)) {
                stack_push(cpu, REG_PC);
                reg_write16(cpu, REG_PC, next_addr);
            }
            else
                tick_MCycle();
            break;
        case JP_C:
            if (flag_reg_read(cpu, C)) {
                stack_push(cpu, REG_PC);
                reg_write16(cpu, REG_PC, next_addr);
            }
            else
                tick_MCycle();
            break;
        default:
            printf("Invalid CALL operand %d \n", instr.op_a);
            exit(0);
    }
}

static void execute_RST(cpu *cpu, instruction instr) {
    stack_push(cpu, REG_PC);
    reg_write16(cpu, REG_PC, instr.op_a);
}

static void execute_RETI(cpu *cpu) {
    stack_pop(cpu, REG_PC);
    tick_MCycle();
    cpu->IME = true;
    tick_MCycle();
}

static void execute_RET(cpu *cpu, instruction instr) {
    switch (instr.op_a) {
        case NONE:
            stack_pop(cpu, REG_PC);
            tick_MCycle();
            tick_MCycle();
            break;
        case JP_NZ:
            if (!flag_reg_read(cpu, Z)) {
                tick_MCycle();
                stack_pop(cpu, REG_PC);
            }
            tick_MCycle();
            tick_MCycle();
            break;
        case JP_Z:
            if (flag_reg_read(cpu, Z)) {
                tick_MCycle();
                stack_pop(cpu, REG_PC);
            }
            tick_MCycle();
            tick_MCycle();
            break;
        case JP_NC:
            if (!flag_reg_read(cpu, C)) {
                tick_MCycle();
                stack_pop(cpu, REG_PC);
            }
            tick_MCycle();
            tick_MCycle();
            break;
        case JP_C:
            if (flag_reg_read(cpu, C)) {
                tick_MCycle();
                stack_pop(cpu, REG_PC);
            }
            tick_MCycle();
            tick_MCycle();
            break;
        default:
            printf("Invalid RET instruction \n");
            exit(0);
    }
}

static void handle_interrupts(cpu *cpu) {
    val16 PC;
    u8 IE;
    u8 IFandIE;

    PC = (val16)cpu->PC;
    // disable interrupt
    cpu->IME = 0;
    tick_MCycle();
    tick_MCycle();
    // push PC to stack
    bus_write(--cpu->SP, PC.msb, true);
    // if the IE changes at this point, it doesn't affect the interrupt handling
    IE = IE_register;
    bus_write(--cpu->SP, PC.lsb, true);

    // if - else if to achieve interrupt priority
    // the last 2 steps of interrupt handling is to
    // first disable the specific interrupt(set the
    // proper bit of IF register to 0) and then jump to
    // interrupt address
    // TODO use builtin functions to find highest set bit
    IFandIE = IF_register & IE;
    if (bit_read(IFandIE, 0)) {
        // VBLANK interrupt
        bit_clear(&IF_register, 0);
        reg_write16(cpu, REG_PC, 0x0040);
    }
    else if (bit_read(IFandIE, 1)) {
        // LCDstat interrupt
        bit_clear(&IF_register, 1);
        reg_write16(cpu, REG_PC, 0x0048);
    }
    else if (bit_read(IFandIE, 2)) {
        // Timer Interrupt
        bit_clear(&IF_register, 2);
        reg_write16(cpu, REG_PC, 0x0050);
    }
    else if (bit_read(IFandIE, 3)) {
        // Serial interrupt
        bit_clear(&IF_register, 3);
        reg_write16(cpu, REG_PC, 0x0058);
    }
    else if (bit_read(IFandIE, 4)) {
        // Joypad interrupt
        bit_clear(&IF_register, 4);
        reg_write16(cpu, REG_PC, 0x0060);
    }
    else {
        // cancelled intr
        reg_write16(cpu, REG_PC, 0x0000);
    }
    tick_MCycle();
}

#ifdef TEST_CHECK
static void checkMooneyeTest(cpu *cpu, u8 opcode) {
    if (opcode == 0x40) {
        if (cpu->B == 3 && cpu->C == 5 && cpu->D == 8 && cpu->E == 13 && cpu->H == 21 && cpu->L == 34) {
            printf("TEST SUCCESSFUL \n");
            testPassed = 1;
        }
    }
}
#endif

#ifdef DEBUG
static instruction fetch_instruction(cpu *cpu, FILE *logFile) {
#else
static instruction fetch_instruction(cpu *cpu) {
#endif
    u8 opcode = bus_read(cpu->PC, false);

#ifdef TEST_CHECK
    checkMooneyeTest(cpu, opcode);
#endif

    if (!cpu->isHaltBug)
        cpu->PC++;
    else
        cpu->isHaltBug = false;

    instruction instr = opcode_to_instr(opcode, cpu->isCB);
#ifdef DEBUG
    fprintf(logFile, "op code: 0x%02X ", opcode);
#endif
    return instr;
}

static void execute(cpu *cpu, instruction instr) {
    if (!cpu->isCB) {
        switch (instr.type) {
            case LD:
            case LD_SP_HL:
            case LDD:
            case LDI:
            case LDHL:
                execute_LD(cpu, instr);
                break;
            case PUSH:
                stack_push(cpu, instr.op_a);
                break;
            case POP:
                stack_pop(cpu, instr.op_a);
                tick_MCycle();
                break;
            case ADD16:
                execute_ADD16(cpu, instr);
                break;
            case ADD:
                execute_ADD(cpu, instr);
                break;
            case ADC:
                execute_ADC(cpu, instr);
                break;
            case SUB:
                execute_SUB(cpu, instr);
                break;
            case SBC:
                execute_SBC(cpu, instr);
                break;
            case AND:
                execute_AND(cpu, instr);
                break;
            case OR:
                execute_OR(cpu, instr);
                break;
            case XOR:
                execute_XOR(cpu, instr);
                break;
            case CP:
                execute_CP(cpu, instr);
                break;
            case INC16:
                execute_INC16(cpu, instr);
                break;
            case INC:
                execute_INC(cpu, instr);
                break;
            case DEC16:
                execute_DEC16(cpu, instr);
                break;
            case DEC:
                execute_DEC(cpu, instr);
                break;
            case DAA:
                execute_DAA(cpu);
                break;
            case CPL:
                execute_CPL(cpu);
                break;
            case CCF:
                execute_CCF(cpu);
                break;
            case SCF:
                execute_SCF(cpu);
                break;
            case NOP:
                execute_NOP();
                break;
            case HALT:
                execute_HALT(cpu);
                break;
            case STOP:
                execute_STOP();
                break;
            case DI:
                execute_DI(cpu);
                break;
            case EI:
                execute_EI(cpu);
                break;
            case RLCA:
                execute_RLCA(cpu);
                break;
            case RLA:
                execute_RLA(cpu);
                break;
            case RRCA:
                execute_RRCA(cpu);
                break;
            case RRA:
                execute_RRA(cpu);
                break;
            case JP:
                execute_JP(cpu, instr);
                break;
            case JR:
                execute_JR(cpu, instr);
                break;
            case CALL:
                execute_CALL(cpu, instr);
                break;
            case RST:
                execute_RST(cpu, instr);
                break;
            case RET:
                execute_RET(cpu, instr);
                break;
            case RETI:
                execute_RETI(cpu);
                break;
            case CB:
                cpu->isCB = true;
                return;
            case ADDSP:
                execute_ADDSP(cpu);
                break;
            default:
                printf("Invalid unprefixed instruction %d \n", instr.type);
                exit(0);
        }
    }
    else {
        switch (instr.type) {
            case SWAP:
                execute_SWAP(cpu, instr);
                break;
            case RLC:
                execute_RLC(cpu, instr);
                break;
            case RL:
                execute_RL(cpu, instr);
                break;
            case RRC:
                execute_RRC(cpu, instr);
                break;
            case RR:
                execute_RR(cpu, instr);
                break;
            case SLA:
                execute_SLA(cpu, instr);
                break;
            case SRA:
                execute_SRA(cpu, instr);
                break;
            case SRL:
                execute_SRL(cpu, instr);
                break;
            case BIT:
                execute_BIT(cpu, instr);
                break;
            case SET:
                execute_SET(cpu, instr);
                break;
            case RES:
                execute_RES(cpu, instr);
                break;
            default:
                printf("Invalid prefixed instruction %d \n", instr.type);
                exit(0);
        }
        cpu->isCB = false;
    }
};

void cpu_init() {
    bool isChecksumNonZero = bus_read(0x014D, false) != 0;

    _cpu.A = 0x01;
    _cpu.F = 0xB0;
    flag_reg_write(&_cpu, H, isChecksumNonZero);
    flag_reg_write(&_cpu, C, isChecksumNonZero);
    _cpu.B = 0x00;
    _cpu.C = 0x13;
    _cpu.D = 0x00;
    _cpu.E = 0xD8;
    _cpu.H = 0x01;
    _cpu.L = 0x4D;
    _cpu.PC = 0x100;
    _cpu.SP = 0xFFFE;

    _cpu.isCB = false;
    _cpu.isHaltBug = false;
    _cpu.isHalted = false;
    _cpu.IME = false;
    _cpu.scheduledIME = false;

    IE_register = 0;
    IF_register = 0xE1;
}

#ifdef DEBUG
void cpu_run(FILE *logFile) {
#else
void cpu_run() {
#endif
    instruction currInstr;

    joypad_readInput();
    // check if cpu is halted
    if (_cpu.isHalted) {
        if ((IE_register & IF_register & 0x1F) != 0) {
            _cpu.isHalted = false;
        }
        else {
            tick_MCycle();
            return;
        }
    }

    // handle interrupts
    if (_cpu.IME && ((IE_register & IF_register & 0x1F) != 0)) {
        handle_interrupts(&_cpu);
    }

    // set the IME flag to 1 if scheduled
    if (_cpu.scheduledIME) {
        _cpu.scheduledIME = false;
        _cpu.IME = true;
    }

    // fetch instruction
#ifdef DEBUG
    currInstr = fetch_instruction(&_cpu, logFile);
#else
    currInstr = fetch_instruction(&_cpu);
#endif
    // execute
    execute(&_cpu, currInstr);

#ifdef DEBUG
    reg_print(logFile, _cpu);
#endif
}
