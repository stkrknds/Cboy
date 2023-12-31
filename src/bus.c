#include "bus.h"
#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "timing.h"

// TODO move them out of here
static u8 WORK_RAM[0x2000];
static u8 HRAM[0x7F];

#ifdef TEST_CHECK
static void printBlarggTest(u16 addr, u8 data) {
    static u8 blarggBYTE;

    if (addr == 0xFF01) {
        blarggBYTE = data;
        return;
    }

    if (addr == 0xFF02 && data == 0x81) {
        printf("%c", blarggBYTE);
    }
}
#endif

u8 bus_read(u16 addr, bool tick) {
    // when memory is unreachable return 0xFF
    u8 val = 0xFF;

    if (tick)
        tick_TCycles(4);

    // if(oam.state == ACTIVE && (addr < 0xFF80 || addr == 0xFFFF)){
    //     val = oam.currTransByte;
    //     goto incr1;
    // }

    if (addr < 0x8000)
        val = (*cartridge_read)(addr);
    else if (addr < 0xA000)
        val = VRAM[addr - 0x8000];
    else if (addr < 0xC000)
        val = (*cartridge_read)(addr);
    else if (addr < 0xE000)
        val = WORK_RAM[addr - 0xC000];
    else if (addr < 0xFE00)
        printf("Prohibited memory area, addr = 0x%04X \n", addr);
    else if (addr < 0xFEA0)
        val = oam_read(addr);
    else if (addr < 0xFF00)
        printf("Prohibited memory area, addr = 0x%04X \n", addr);
    else if (addr < 0xFF80) {
        if (addr == 0xFF00)
            val = joypad_read();
        else if (addr >= 0xFF03 && addr <= 0xFF07)
            val = timers_read(addr);
        else if (addr == 0xFF0F)
            val = IF_register;
        else if (addr == 0xFF24)
            val = NR50_register;
        else if (addr >= 0xFF40 && addr <= 0xFF4B)
            val = read_ppu(addr);
    }
    else if (addr < 0xFFFF)
        val = HRAM[addr - 0xFF80];
    else
        val = IE_register;

    return val;
}

void bus_write(u16 addr, u8 data, bool tick) {
    if (tick)
        tick_TCycles(4);

    if (addr < 0x8000)
        (*cartridge_write)(addr, data);
    else if (addr < 0xA000)
        VRAM[addr - 0x8000] = data;
    else if (addr < 0xC000)
        (*cartridge_write)(addr, data);
    else if (addr < 0xE000)
        WORK_RAM[addr - 0xC000] = data;
    else if (addr < 0xFE00)
        printf("Prohibited memory area, addr = 0x%04X \n", addr);
    else if (addr < 0xFEA0)
        oam_write(addr, data);
    else if (addr < 0xFF00)
        printf("Prohibited memory area, addr = 0x%04X \n", addr);
    else if (addr < 0xFF80) {
        if (addr == 0xFF00)
            joypad_write(data);
        else if (addr == 0xFF01 || addr == 0xFF02) {
#ifdef TEST_CHECK
            printBlarggTest(addr, data);
#endif
        }
        else if (addr >= 0xFF03 && addr <= 0xFF07)
            timers_write(addr, data);
        else if (addr == 0xFF0F)
            IF_register = data | 0xE0;
        else if (addr == 0xFF24)
            NR50_register = data;
        else if (addr >= 0xFF40 && addr <= 0xFF4B)
            write_ppu(addr, data);
    }
    else if (addr < 0xFFFF)
        HRAM[addr - 0xFF80] = data;
    else
        IE_register = data;
}
