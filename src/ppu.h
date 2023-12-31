#ifndef PPU_H
#define PPU_H

#include "types.h"

#include <stdbool.h>

typedef enum { BGP, OBP0, OBP1 } palette;
typedef enum ADDRESSING_MODE { MODE_8000, MODE_8800 } ADDRESING_MODE;
typedef enum PPU_MODE { MODE_0, MODE_1, MODE_2, MODE_3 } PPU_MODE;
typedef enum PIXELFETCHER_STATE { fetchTileNo, fetchTileDataLow, fetchTileDataHigh, pushToFifo } PIXELFETCHER_STATE;
typedef enum PIXELMIXER_STATE { STALLED, P_ACTIVE } PIXELMIXER_STATE;
typedef enum currFetching { BACKGROUND, WINDOW, SPRITE } PIXELFETCHER_currFetching;
typedef enum DMA_OAM_STATE { INACTIVE, ACTIVE } DMA_OAM_STATE;

typedef struct sprite {
    // actual Y = Y - 16
    u8 Y;
    // actual X = X - 8
    u8 X;
    u8 tileNumber;
    u8 flags;
} sprite;

typedef struct spriteBuffer {
    sprite sprites[10];
    u8 numStoredSprites;
} spriteBuffer;

typedef struct {
    u8 LY_register;
    u8 X_position;
    // viewport offset
    u8 SCY_register;
    u8 SCX_register;
    // window offset
    u8 WX_register;
    u8 WY_register;

    u8 LCDC_register;
    u8 LYC_register;
    u8 STAT_register;
    u8 BGP_register;
    u8 OBP0_register;
    u8 OBP1_register;
    spriteBuffer spriteBuffer;

    u16 scanLineTicks;
    u16 MODE2addr;

    PPU_MODE currMode;

    bool isSecondCycle;
    bool WY_equal_LY;
    bool stat_OR;
    bool firstTimeInScanline;
    bool triggerVBLANKintr;
} ppu;

typedef struct {
    u8 colorNumber;
    palette palette;
    u8 spritePriority;
    bool backgroundPriority;
} pixelInFIFO;

typedef struct {
    PIXELMIXER_STATE state;
    pixelInFIFO backgroundPixel;
    pixelInFIFO spritePixel;

    u8 waitNumCycles;
} pixelMixer;

// typical ring/circular buffer
typedef struct {
    pixelInFIFO pixels[8];

    u8 startIdx;
    u8 endIdx;
    u8 numStoredPixels;
} FIFO;

typedef struct spriteToFetch {
    sprite s;
    bool isCurrentlyFetching;
    bool alreadyAsked[10];
} spriteToFetch;

typedef struct pixelFetcher {
    u8 X_position;
    u8 WINDOW_LINE_COUNTER;
    u8 fetchedTileNumber;
    u8 fetchedRowLow;
    u8 fetchedRowHigh;

    u16 fetchTileAddr;

    bool firstTimeInScanline;
    bool incrWINDOW;
    bool isSecondCycle;

    PIXELFETCHER_currFetching currFetching;

    PIXELFETCHER_STATE state;

    spriteToFetch spriteToFetch;
} pixelFetcher;

typedef struct OAM {
    u8 memory[0xA0];
    u8 waitNumCycles;
    u8 currTransByte;
    u8 DMA_CTR_REGISTER;

    u16 sourceAddr;
    u16 destAddr;

    DMA_OAM_STATE state;
} OAM;

extern ppu _ppu;
extern u8 VRAM[0x2000];

u8 oam_read(u16 addr);
void oam_write(u16 addr, u8 data);
u8 read_ppu(u16 addr);
void write_ppu(u16 addr, u8 data);
void ppu_init();
void ppu_tick();

#endif // PPU_H
