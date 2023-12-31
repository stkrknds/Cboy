#include "cartridge.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

cartridge cart;
MBC1_chip *mbc1 = NULL;
MBC3_chip *mbc3 = NULL;

u8 (*cartridge_read)(u16);
void (*cartridge_write)(u16, u8);

static void ramSizeError() {
    printf("RAM size not compatible with MBC type. \n");
    exit(0);
}

static u8 MBCnone_read(u16 addr) {
    assert(addr < 0x8000 || (addr >= 0xA000 && addr <= 0xBFFF));
    if (addr < 0x8000) {
        return cart.loadedFile[addr];
    }
    return 0xFF;
}

static u8 MBC1_read(u16 addr) {
    assert(addr < 0x8000 || (addr >= 0xA000 && addr <= 0xBFFF));
    if (addr < 0x4000) {
        if (!mbc1->mode)
            return cart.loadedFile[addr];
        else
            return cart.loadedFile[0x4000 * mbc1->zeroBankNum + addr];
    }
    else if (addr < 0x8000) {
        return cart.loadedFile[0x4000 * mbc1->highBankNum + (addr - 0x4000)];
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (mbc1->ramEnable) {
            switch (cart.RAMtype) {
                case NO_RAM:
                    return 0xFF;
                case _2K_RAM:
                    return cart.externalRAM[(addr - 0x4000) % 2048];
                case _8K_RAM:
                    return cart.externalRAM[(addr - 0x4000) % 8192];
                case _32K_RAM:
                    if (mbc1->mode)
                        return cart.externalRAM[0x2000 * mbc1->ramBankNum + (addr - 0xA000)];
                    return cart.externalRAM[addr - 0xA000];
                default:
                    ramSizeError();
            }
        }
    }
    return 0xFF;
}

static u8 MBC3_read(u16 addr) {
    assert(addr < 0x8000 || (addr >= 0xA000 && addr <= 0xBFFF));
    if (addr < 0x4000)
        return cart.loadedFile[addr];
    else if (addr < 0x8000)
        return cart.loadedFile[0x4000 * mbc3->romBankNum + (addr - 0x4000)];
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (mbc3->ramEnable)
            return cart.externalRAM[0x2000 * mbc3->ramBankNum + (addr - 0xA000)];
    }
    return 0xFF;
}

static void MBCnone_write(u16 addr, u8 data) {
    // do nothing
}

static void MBC1_write(u16 addr, u8 data) {
    if (addr < 0x2000)
        mbc1->ramEnable = (data & 0x0F) == 0x0A;
    else if (addr < 0x4000) {
        mbc1->romBankNum = (data != 0) ? (data & mbc1->mask) : 1;

        if (mbc1->numRomBanks < 64)
            mbc1->highBankNum = mbc1->romBankNum;
        else if (mbc1->numRomBanks == 64)
            mbc1->highBankNum = mbc1->romBankNum | ((mbc1->ramBankNum & 0x01) << 5);
        else if (mbc1->numRomBanks == 128)
            mbc1->highBankNum = mbc1->romBankNum | ((mbc1->ramBankNum & 0x03) << 5);
    }
    else if (addr < 0x6000) {
        mbc1->ramBankNum = data & 0x03;
        // update zero bank num
        if (mbc1->numRomBanks == 64)
            // TODO MBC1M
            mbc1->zeroBankNum |= ((mbc1->ramBankNum & 0x01) << 5);
        else if (mbc1->numRomBanks == 128)
            mbc1->zeroBankNum |= (mbc1->ramBankNum << 5);
    }
    else if (addr < 0x8000)
        mbc1->mode = data & 1;
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (mbc1->ramEnable) {
            switch (cart.RAMtype) {
                case NO_RAM:
                    break;
                case _2K_RAM:
                    cart.externalRAM[(addr - 0x4000) % 2048] = data;
                    break;
                case _8K_RAM:
                    cart.externalRAM[(addr - 0x4000) % 8192] = data;
                    break;
                case _32K_RAM:
                    if (mbc1->mode)
                        cart.externalRAM[0x2000 * mbc1->ramBankNum + (addr - 0xA000)] = data;
                    else
                        cart.externalRAM[addr - 0xA000] = data;
                    break;
                default:
                    ramSizeError();
            }
        }
    }
}

static void MBC3_write(u16 addr, u8 data) {
    if (addr < 0x2000)
        mbc3->ramEnable = (data & 0x0F) == 0x0A;
    else if (addr < 0x4000) {
        u8 rdata = data & 0x7F;
        mbc3->romBankNum = (rdata == 0) ? 1 : rdata;
    }
    else if (addr < 0x6000) {
        if (data <= 0x03)
            mbc3->ramBankNum = data & 0x03;
        // TODO RTC
    }
    else if (addr < 0x8000)
        // TODO RTC
        printf("RTC NOT IMPLEMENTED! \n");
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (mbc3->ramEnable)
            cart.externalRAM[0x2000 * mbc3->ramBankNum + (addr - 0xA000)] = data;
    }
}

static void MBC1_init() {
    mbc1->mode = 0;
    mbc1->ramBankNum = 0;
    mbc1->romBankNum = 1;
    mbc1->ramEnable = false;
    mbc1->zeroBankNum = 0;
    mbc1->highBankNum = 1;
    assert(mbc1->numRomBanks > 0);
    mbc1->mask = (mbc1->numRomBanks - 1) & 0b11111;
}

static void MBC3_init() {
    // TODO RTC
    mbc3->ramBankNum = 0;
    mbc3->romBankNum = 1;
    mbc3->ramEnable = false;
}

static void externalRAM_init(cartridge *cart) {
    switch (cart->RAMtype) {
        case NO_RAM:
            cart->RAMsize = 0;
            cart->externalRAM = NULL;
            return;
        case _2K_RAM:
            cart->RAMsize = 2 * 1024;
            break;
        case _8K_RAM:
            cart->RAMsize = 8 * 1024;
            break;
        case _32K_RAM:
            cart->RAMsize = 32 * 1024;
            break;
        case _128K_RAM:
            cart->RAMsize = 128 * 1024;
            break;
        case _64K_RAM:
            cart->RAMsize = 64 * 1024;
            break;
    }

    cart->externalRAM = (u8 *)malloc(cart->RAMsize);

    // load RAM
    if (cart->MBCtype == MBC1_BAT || cart->MBCtype == MBC3_BAT || cart->MBCtype == MBC3_RTC_BAT) {
        strncpy(cart->savFileName, (char *)cart->title, 16);
        strcat(cart->savFileName, ".sav");
        if (access(cart->savFileName, F_OK) == 0) {
            FILE *fp;
            fp = fopen(cart->savFileName, "r");

            if (fp == NULL) {
                printf("Error while opening sav file: %s", cart->savFileName);
                exit(-1);
            }

            fread(cart->externalRAM, cart->RAMsize, 1, fp);
            fclose(fp);
        }
    }
}

static void MBC_free() {
    switch (cart.MBCtype) {
        default:
        case MBC_NONE:
            break;
        case MBC1_RAM:
        case MBC1_BAT:
        case MBC1:
            free(mbc1);
            break;
        case MBC3_BAT:
            free(mbc3);
            break;
    }
}

void cartridge_load(char *filePath) {
    FILE *ptr;
    long int fileSize;

    ptr = fopen(filePath, "rb");

    if (ptr == NULL) {
        printf("Cannot open file: %s", filePath);
        exit(-1);
    }

    // get file size
    fseek(ptr, 0L, SEEK_END);
    fileSize = ftell(ptr);

    cart.loadedFile = (u8 *)malloc(fileSize);

    rewind(ptr);

    fread(cart.loadedFile, fileSize, 1, ptr);

    fclose(ptr);

    cart.MBCtype = (MBC_TYPE)cart.loadedFile[0x0147];
    cart.RAMtype = (RAM_TYPE)cart.loadedFile[0x0149];
    cart.title = &cart.loadedFile[0x0134];

    externalRAM_init(&cart);

    switch (cart.MBCtype) {
        case MBC_NONE:
            cartridge_read = &MBCnone_read;
            cartridge_write = &MBCnone_write;
            break;
        case MBC1:
        case MBC1_RAM:
        case MBC1_BAT:
            mbc1 = (MBC1_chip *)malloc(sizeof(MBC1_chip));
            mbc1->numRomBanks = (u32)fileSize / 16384;
            MBC1_init();
            cartridge_read = &MBC1_read;
            cartridge_write = &MBC1_write;
            break;
        case MBC3_BAT:
            mbc3 = (MBC3_chip *)malloc(sizeof(MBC3_chip));
            MBC3_init();
            cartridge_read = &MBC3_read;
            cartridge_write = &MBC3_write;
            break;
        default:
            printf("MBC type 0x%02X not supported. \n", cart.MBCtype);
            exit(0);
    }
};

void cartridge_free() {
    free(cart.loadedFile);

    if (cart.MBCtype == MBC1_BAT || cart.MBCtype == MBC3_BAT) {
        FILE *ptr;
        ptr = fopen(cart.savFileName, "w");

        if (ptr == NULL) {
            printf("Couldn't write to file: %s \n", cart.savFileName);
            exit(-1);
        }

        fwrite(cart.externalRAM, cart.RAMsize, 1, ptr);
        fclose(ptr);
    }

    if (cart.externalRAM != NULL)
        free(cart.externalRAM);

    MBC_free();
}
