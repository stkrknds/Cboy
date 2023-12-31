#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "types.h"

#include <stdbool.h>

typedef enum { MBC_NONE = 0x00, MBC1 = 0x01, MBC1_RAM = 0x02, MBC1_BAT = 0x03, MBC3_RTC = 0x0F, MBC3_RTC_BAT = 0x10, MBC3 = 0x11, MBC3_RAM = 0x12, MBC3_BAT = 0x13 } MBC_TYPE;

typedef enum {
    NO_RAM = 0x00,
    _2K_RAM = 0x01,
    _8K_RAM = 0x02,
    _32K_RAM = 0x03,
    _128K_RAM = 0x04,
    _64K_RAM = 0x05,
} RAM_TYPE;

typedef struct {
    u8 *loadedFile;
    u8 *externalRAM;

    MBC_TYPE MBCtype;
    RAM_TYPE RAMtype;
    u32 RAMsize;
    u8 *title;
    char savFileName[20];
} cartridge;

typedef struct {
    u8 romBankNum;
    u8 ramBankNum;
    bool mode;
    bool ramEnable;
    u32 numRomBanks;

    u8 zeroBankNum;
    u8 highBankNum;

    u8 mask;
} MBC1_chip;

typedef struct {
    u8 romBankNum;
    u8 ramBankNum;
    bool ramEnable;
} MBC3_chip;

void cartridge_load(char *filePath);
void cartridge_free();
extern u8 (*cartridge_read)(u16);
extern void (*cartridge_write)(u16, u8);

#endif // CARTRIDGE_H
