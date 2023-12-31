#include "ppu.h"
#include "bus.h"
#include "cpu.h"
#include "screen.h"

#include <assert.h>

ppu _ppu;
static pixelFetcher _pixelFetcher;
static pixelMixer _pixelMixer;
static FIFO backgroundFIFO;
static FIFO spriteFIFO;
static OAM oam;
u8 VRAM[0x2000];

static const u16 OAMstartingAddr = 0xFE00;

static void DMA_writeREG(OAM *oam, u8 data) {
    oam->DMA_CTR_REGISTER = data;
    oam->state = ACTIVE;
    oam->sourceAddr = (oam->DMA_CTR_REGISTER << 8) & 0xFF00;
    oam->destAddr = OAMstartingAddr;
    oam->waitNumCycles = 4;
}

static void DMA_tick(OAM *oam) {
    switch (oam->state) {
        case INACTIVE:
            return;
        case ACTIVE:
            if (oam->waitNumCycles != 0) {
                oam->waitNumCycles--;
                return;
            }
            oam->currTransByte = bus_read(oam->sourceAddr++, false);
            bus_write(oam->destAddr++, oam->currTransByte, false);

            if (oam->destAddr > 0xFE9F) {
                oam->state = INACTIVE;
            }
            oam->waitNumCycles = 3;
    }
}

static void printInvalidAddr(u16 addr) {
    printf("ERROR invalid ppu addr 0x%02X. \n", addr);
    exit(0);
}

static void FIFO_reset(FIFO *FIFO) {
    FIFO->numStoredPixels = 0;
    FIFO->startIdx = 0;
    FIFO->endIdx = 0;
}

static void FIFO_pop(FIFO *FIFO, pixelInFIFO *out) {
    assert(FIFO->numStoredPixels != 0);
    assert(FIFO->startIdx < 8);
    *out = FIFO->pixels[FIFO->startIdx];
    FIFO->startIdx = (FIFO->startIdx + 1) % 8;
    FIFO->numStoredPixels--;
}

static void FIFO_placeSpritePixel(FIFO *FIFO, u8 colorNumber, sprite s, u8 idx) {
    FIFO->pixels[idx].colorNumber = colorNumber;
    FIFO->pixels[idx].palette = (!bit_read(s.flags, 4)) ? OBP0 : OBP1;
    FIFO->pixels[idx].backgroundPriority = bit_read(s.flags, 7);
}

static void pixelFetcher_setState(pixelFetcher *pixelFetcher, PIXELFETCHER_STATE state) {
    pixelFetcher->state = state;
    pixelFetcher->isSecondCycle = false;
}

static void scanLineEndReset(ppu *ppu, pixelFetcher *pixelFetcher, pixelMixer *pixelMixer, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    ppu->MODE2addr = OAMstartingAddr;
    ppu->X_position = 0;
    ppu->scanLineTicks = 0;
    ppu->LY_register++;
    ppu->spriteBuffer.numStoredSprites = 0;
    ppu->firstTimeInScanline = true;

    pixelFetcher->X_position = 0;
    pixelFetcher_setState(pixelFetcher, fetchTileNo);
    pixelFetcher->firstTimeInScanline = true;
    pixelFetcher->currFetching = BACKGROUND;
    pixelFetcher->spriteToFetch.isCurrentlyFetching = false;
    pixelFetcher->incrWINDOW = false;
    for (u8 i = 0; i < 10; i++)
        pixelFetcher->spriteToFetch.alreadyAsked[i] = false;

    pixelMixer->state = STALLED;

    FIFO_reset(backgroundFIFO);
    FIFO_reset(spriteFIFO);
}

static sprite readSprite(u16 addr) {
    sprite s;

    s.Y = bus_read(addr++, false);
    s.X = bus_read(addr++, false);
    s.tileNumber = bus_read(addr++, false);
    s.flags = bus_read(addr, false);

    return s;
};

static void storeSpriteInBuffer(sprite s, ppu *ppu) { ppu->spriteBuffer.sprites[ppu->spriteBuffer.numStoredSprites++] = s; };

static bool isTallSprite(ppu ppu) { return bit_read(ppu.LCDC_register, 2); };

static ADDRESING_MODE whatAddrMode(ppu ppu) { return (bit_read(ppu.LCDC_register, 4)) ? MODE_8000 : MODE_8800; };

static bool BGTileMapSelect(ppu ppu) {
    // If it is set to 1, the background map located at $9C00-$9FFF is used, otherwise it uses the one at $9800-$9BFF
    return bit_read(ppu.LCDC_register, 3);
};

static bool windowTileMapSelect(ppu ppu) {
    // If it is set to 1, the background map located at $9C00-$9FFF is used, otherwise it uses the one at $9800-$9BFF
    return bit_read(ppu.LCDC_register, 6);
}

static u8 getPixelFromRow(u8 rowLow, u8 rowHigh, u8 idx) {
    bool low = bit_read(rowLow, idx);
    bool high = bit_read(rowHigh, idx);
    return (high << 1) | low;
}

static bool isInsideWindow(ppu *ppu) { return bit_read(ppu->LCDC_register, 5) && ppu->WY_equal_LY && (ppu->X_position + 7 >= ppu->WX_register); }

static u8 decodePixel(pixelInFIFO pixel, ppu ppu) {
    u8 reg;

    switch (pixel.palette) {
        default:
        case BGP:
            reg = ppu.BGP_register;
            break;
        case OBP0:
            reg = ppu.OBP0_register;
            break;
        case OBP1:
            reg = ppu.OBP1_register;
            break;
    }

    assert(pixel.colorNumber < 4);

    // if color is 0 we need the 2 lsbs
    // if color is 1 we need the next 2 bits left of the 2 lsbs(bits [3:1])
    // so we need to shift the palette register right by colorNumber * 2 bits
    return (reg >> (pixel.colorNumber * 2)) & 0x03;
}

// HBLANK
static void ppu_MODE0_tick(ppu *ppu, pixelFetcher *pixelFetcher, pixelMixer *pixelMixer, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    if (ppu->scanLineTicks < 455)
        ppu->scanLineTicks++;
    else {
        assert(ppu->scanLineTicks == 455);
        if (ppu->LY_register == 143) {
            ppu->currMode = MODE_1;
            // VBLANK interrupt
            ppu->triggerVBLANKintr = true;
        }
        else {
            ppu->currMode = MODE_2;
            if (pixelFetcher->incrWINDOW) {
                pixelFetcher->WINDOW_LINE_COUNTER++;
            }
        }

        scanLineEndReset(ppu, pixelFetcher, pixelMixer, backgroundFIFO, spriteFIFO);
    }
};

// VBLANK
static void ppu_MODE1_tick(ppu *ppu, pixelFetcher *pixelFetcher) {
    if (ppu->scanLineTicks < 455)
        ppu->scanLineTicks++;
    else {
        assert(ppu->scanLineTicks == 455);
        // last scanline
        if (ppu->LY_register == 153) {
            ppu->currMode = MODE_2;
            ppu->WY_equal_LY = false;
            ppu->LY_register = 0;
            pixelFetcher->WINDOW_LINE_COUNTER = 0;
        }
        else
            ppu->LY_register++;
        ppu->scanLineTicks = 0;
    }
};

static void ppu_MODE2_tick(ppu *ppu) {
    if (!ppu->isSecondCycle) {
        sprite s = readSprite(ppu->MODE2addr);
        u8 spriteHeight = (isTallSprite(*ppu)) ? 16 : 8;

        if (s.X > 0 && (ppu->LY_register + 16) >= s.Y && (ppu->LY_register + 16) < (s.Y + spriteHeight) && ppu->spriteBuffer.numStoredSprites < 10)
            storeSpriteInBuffer(s, ppu);

        ppu->isSecondCycle = true;
        ppu->MODE2addr += 4;
    }
    else {
        if (ppu->MODE2addr >= 0xFE9F) {
            assert(ppu->scanLineTicks == 79);
            ppu->currMode = MODE_3;
        }
        ppu->isSecondCycle = 0;
    }
    ppu->scanLineTicks++;
};

static void pixelFetcher_fetchTileNum_tick(ppu *ppu, pixelFetcher *pixelFetcher) {
    if (!pixelFetcher->isSecondCycle) {
        u8 tileNumber = 0;

        switch (pixelFetcher->currFetching) {
            case BACKGROUND: {
                u8 fetchX = (ppu->SCX_register / 8 + pixelFetcher->X_position) & 0x1F;
                u8 fetchY = ((ppu->SCY_register + ppu->LY_register) & 0xFF) / 8;
                u16 startingAddr = (BGTileMapSelect(*ppu)) ? 0x9C00 : 0x9800;

                assert(fetchX < 32);
                assert(fetchY < 32);

                u16 addr = startingAddr + ((32 * fetchY + fetchX) & 0x3FF);

                tileNumber = bus_read(addr, false);
                break;
            }
            case WINDOW: {
                u8 fetchX = pixelFetcher->X_position;
                u8 fetchY = pixelFetcher->WINDOW_LINE_COUNTER / 8;
                u16 startingAddr = (windowTileMapSelect(*ppu)) ? 0x9C00 : 0x9800;

                assert(fetchX < 32);
                assert(fetchY < 32);

                u16 addr = startingAddr + 32 * fetchY + fetchX;

                tileNumber = bus_read(addr, false);
                break;
            }
            case SPRITE: {
                if (isTallSprite(*ppu)) {
                    u8 tileRow = ppu->LY_register - (pixelFetcher->spriteToFetch.s.Y - 16);
                    bool isSpriteVFlipped = bit_read(pixelFetcher->spriteToFetch.s.flags, 6);
                    // if the sprite isn't flipped and row < 8 or
                    // if the sprite is flipped(top tile at bottom) and row > 7
                    // need to fetch the top tile of the sprite
                    // else fetch bot tile
                    if ((tileRow < 8 && !isSpriteVFlipped) || (tileRow > 7 && isSpriteVFlipped))
                        // if top tile, lsb is 0
                        tileNumber = pixelFetcher->spriteToFetch.s.tileNumber & 0xFE;
                    else
                        // if bot tile, lsb is 1
                        tileNumber = pixelFetcher->spriteToFetch.s.tileNumber | 1;
                }
                else
                    tileNumber = pixelFetcher->spriteToFetch.s.tileNumber;
                break;
            }
        }

        pixelFetcher->fetchedTileNumber = tileNumber;
        pixelFetcher->isSecondCycle = true;
    }
    else
        pixelFetcher_setState(pixelFetcher, fetchTileDataLow);
}

static void pixelFetcher_fetchTileRowLow_tick(ppu *ppu, pixelFetcher *pixelFetcher) {
    if (!pixelFetcher->isSecondCycle) {
        ADDRESING_MODE mode = (pixelFetcher->currFetching == SPRITE) ? MODE_8000 : whatAddrMode(*ppu);
        u16 offset;
        u16 addr;

        switch (pixelFetcher->currFetching) {
            default:
            case BACKGROUND:
                offset = 2 * ((ppu->LY_register + ppu->SCY_register) % 8);
                break;
            case WINDOW:
                offset = 2 * (pixelFetcher->WINDOW_LINE_COUNTER % 8);
                break;
            case SPRITE: {
                bool isSpriteVFlipped = bit_read(pixelFetcher->spriteToFetch.s.flags, 6);

                offset = 2 * ((ppu->LY_register - (pixelFetcher->spriteToFetch.s.Y - 16)) % 8);

                if (isSpriteVFlipped)
                    offset = 14 - offset;
                break;
            }
        }

        switch (mode) {
            case MODE_8000:
                addr = 0x8000 + 16 * pixelFetcher->fetchedTileNumber;
                break;
            case MODE_8800:
                addr = 0x9000 + 16 * ((int8)pixelFetcher->fetchedTileNumber);
                break;
        }

        addr += offset;

        pixelFetcher->fetchTileAddr = addr;
        pixelFetcher->fetchedRowLow = bus_read(addr, false);

        pixelFetcher->isSecondCycle = 1;
    }
    else
        pixelFetcher_setState(pixelFetcher, fetchTileDataHigh);
}

static void pixelFetcher_fetchTileRowHigh_tick(pixelFetcher *pixelFetcher) {
    if (!pixelFetcher->isSecondCycle) {
        pixelFetcher->fetchedRowHigh = bus_read(++pixelFetcher->fetchTileAddr, false);
        pixelFetcher->isSecondCycle = 1;
    }
    else {
        if (!pixelFetcher->firstTimeInScanline)
            pixelFetcher_setState(pixelFetcher, pushToFifo);
        else {
            // the first time the fetcher completes this step in a scanline,
            // its state is fully reset
            pixelFetcher_setState(pixelFetcher, fetchTileNo);
            pixelFetcher->firstTimeInScanline = false;
        }
    }
}

static void pixelFetcher_pushToFIFO_tick(pixelFetcher *pixelFetcher, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    u8 i;

    // push
    if ((pixelFetcher->currFetching == BACKGROUND || pixelFetcher->currFetching == WINDOW)) {
        // if a sprite fetch has been requested, reset and start fetching the sprite
        // TODO Fix this, fails tests
        if (pixelFetcher->spriteToFetch.isCurrentlyFetching) {
            pixelFetcher->state = fetchTileNo;
            pixelFetcher->currFetching = SPRITE;
            return;
        }

        if (backgroundFIFO->numStoredPixels != 0)
            return;

        for (i = 0; i < 8; i++) {
            backgroundFIFO->pixels[backgroundFIFO->endIdx].colorNumber = getPixelFromRow(pixelFetcher->fetchedRowLow, pixelFetcher->fetchedRowHigh, 7 - i);
            backgroundFIFO->pixels[backgroundFIFO->endIdx].palette = BGP;
            backgroundFIFO->endIdx = (backgroundFIFO->endIdx + 1) % 8;
        }

        backgroundFIFO->numStoredPixels = 8;
        pixelFetcher->state = fetchTileNo;
        pixelFetcher->X_position++;
    }
    else if (pixelFetcher->currFetching == SPRITE) {
        u8 pixels[8];

        // a sprite can be partially displayed on the screen(when X < 8)
        u8 numPixelsDisplayed = (pixelFetcher->spriteToFetch.s.X > 7) ? 8 : pixelFetcher->spriteToFetch.s.X;
        u8 displayOffset = 8 - numPixelsDisplayed;
        bool isSpriteHFlipped = bit_read(pixelFetcher->spriteToFetch.s.flags, 5);

        // flip and crop pixels
        if (isSpriteHFlipped) {
            for (i = 0; i < numPixelsDisplayed; i++)
                pixels[i] = getPixelFromRow(pixelFetcher->fetchedRowLow, pixelFetcher->fetchedRowHigh, displayOffset + i);
        }
        else {
            for (i = 0; i < numPixelsDisplayed; i++)
                pixels[i] = getPixelFromRow(pixelFetcher->fetchedRowLow, pixelFetcher->fetchedRowHigh, 7 - (i + displayOffset));
        }

        for (i = 0; i < numPixelsDisplayed; i++) {
            // first check if FIFO slot is empty
            if (i >= spriteFIFO->numStoredPixels) {
                FIFO_placeSpritePixel(spriteFIFO, pixels[i], pixelFetcher->spriteToFetch.s, spriteFIFO->endIdx);
                spriteFIFO->numStoredPixels++;
                spriteFIFO->endIdx = (spriteFIFO->endIdx + 1) % 8;
            }
            else if (spriteFIFO->pixels[(spriteFIFO->startIdx + i) % 8].colorNumber == 0 && pixels[i] != 0) {
                // if not empty, check if the pixel in FIFO's slot is transparent(color == 0)
                // if it is, and the new pixel isn't, replace the pixel in FIFO with the new one
                FIFO_placeSpritePixel(spriteFIFO, pixels[i], pixelFetcher->spriteToFetch.s, (spriteFIFO->startIdx + i) % 8);
            }
        }

        pixelFetcher->state = fetchTileNo;
        pixelFetcher->spriteToFetch.isCurrentlyFetching = false;
        pixelFetcher->currFetching = BACKGROUND;
    }
}

static void pixelFetcher_tick(ppu *ppu, pixelFetcher *pixelFetcher, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    switch (pixelFetcher->state) {
        case fetchTileNo:
            pixelFetcher_fetchTileNum_tick(ppu, pixelFetcher);
            break;
        case fetchTileDataLow:
            pixelFetcher_fetchTileRowLow_tick(ppu, pixelFetcher);
            break;
        case fetchTileDataHigh:
            pixelFetcher_fetchTileRowHigh_tick(pixelFetcher);
            break;
        case pushToFifo:
            pixelFetcher_pushToFIFO_tick(pixelFetcher, backgroundFIFO, spriteFIFO);
            break;
    }
}

static void pixelMixer_tick(ppu *ppu, pixelMixer *pixelMixer, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    switch (pixelMixer->state) {
        case STALLED:
            return;
        case P_ACTIVE: {
            bool isBackgroundEnabled = bit_read(ppu->LCDC_register, 0);

            FIFO_pop(backgroundFIFO, &pixelMixer->backgroundPixel);
            assert(backgroundFIFO->numStoredPixels < 9);

            if (spriteFIFO->numStoredPixels == 0) {
                // If the sprite FIFO doesn't have any pixels, the output pixel is the one shifted out of the background FIFO
                // if background is not enabled, a blank pixel(0) is shifted out
                if (isBackgroundEnabled)
                    pushToScreen(decodePixel(pixelMixer->backgroundPixel, *ppu), ppu->X_position, ppu->LY_register);
                else
                    pushToScreen(0, ppu->X_position, ppu->LY_register);
            }
            else {
                bool isSpriteEnabled = bit_read(ppu->LCDC_register, 1);

                FIFO_pop(spriteFIFO, &pixelMixer->spritePixel);

                if (((pixelMixer->spritePixel.colorNumber == 0 || (pixelMixer->spritePixel.backgroundPriority && (pixelMixer->backgroundPixel.colorNumber != 0))) && isBackgroundEnabled) ||
                    !isSpriteEnabled) {
                    if (isBackgroundEnabled)
                        pushToScreen(decodePixel(pixelMixer->backgroundPixel, *ppu), ppu->X_position, ppu->LY_register);
                    else
                        pushToScreen(0, ppu->X_position, ppu->LY_register);
                }
                else
                    pushToScreen(decodePixel(pixelMixer->spritePixel, *ppu), ppu->X_position, ppu->LY_register);
            }
            ppu->X_position++;
            break;
        }
    }
}

static void ppu_MODE3_tick(ppu *ppu, pixelMixer *pixelMixer, pixelFetcher *pixelFetcher, FIFO *backgroundFIFO, FIFO *spriteFIFO) {
    pixelFetcher_tick(ppu, pixelFetcher, backgroundFIFO, spriteFIFO);

    ppu->scanLineTicks++;

    pixelMixer->state = STALLED;

    if (ppu->WY_register == ppu->LY_register)
        ppu->WY_equal_LY = true;

    // when the background FIFO is empty there is nothing to do
    if (backgroundFIFO->numStoredPixels == 0)
        return;

    // The first time the background is filled in a scanline,
    // SCX%8 pixels must be discarded
    // this consequently extends MODE 3 by a few cycles
    if (ppu->firstTimeInScanline) {
        ppu->firstTimeInScanline = false;

        u8 scx_mod = ppu->SCX_register % 8;

        assert(backgroundFIFO->numStoredPixels > scx_mod);

        for (u8 i = 0; i < scx_mod; i++)
            FIFO_pop(backgroundFIFO, &(pixelMixer->backgroundPixel));

        assert(backgroundFIFO->numStoredPixels < 9);

        // TODO REMOVE HACK
        if (scx_mod >= 1 && scx_mod <= 4) {
            pixelMixer->waitNumCycles = 5;
        }
        else if (scx_mod >= 5 && scx_mod <= 7)
            pixelMixer->waitNumCycles = 9;
        else
            pixelMixer->waitNumCycles = 0;
    }

    // wait
    if (pixelMixer->waitNumCycles != 0) {
        pixelMixer->waitNumCycles--;
        return;
    }

    if (!pixelFetcher->spriteToFetch.isCurrentlyFetching) {
        // Check if sprites need to be fetched
        for (u8 i = 0; i < ppu->spriteBuffer.numStoredSprites; i++) {
            if (ppu->X_position + 8 >= ppu->spriteBuffer.sprites[i].X && !pixelFetcher->spriteToFetch.alreadyAsked[i]) {
                // request sprite fetch
                pixelFetcher->spriteToFetch.s = ppu->spriteBuffer.sprites[i];
                pixelFetcher->spriteToFetch.alreadyAsked[i] = true;
                pixelFetcher->spriteToFetch.isCurrentlyFetching = true;
                return;
            }
        }

        // Check if window tiles need to be fetched
        if (isInsideWindow(ppu) && pixelFetcher->currFetching != WINDOW) {
            pixelFetcher->currFetching = WINDOW;
            pixelFetcher_setState(pixelFetcher, fetchTileNo);
            // we only want to empty the FIFO and reset X position
            // only the first time the window is encountered in a scanline
            // required to pass dmg_acid2 test
            if (!pixelFetcher->incrWINDOW) {
                pixelFetcher->X_position = 0;
                FIFO_reset(backgroundFIFO);
                pixelFetcher->incrWINDOW = true;
            }
            return;
        }

        pixelMixer->state = P_ACTIVE;
    }

    // TODO move this up without failing tests
    pixelMixer_tick(ppu, pixelMixer, backgroundFIFO, spriteFIFO);

    if (ppu->X_position == 160)
        ppu->currMode = MODE_0;
};

static void trigger_intr(ppu *ppu) {
    // TODO update OR only when a value changes
    bit_write(&ppu->STAT_register, 2, ppu->LYC_register == ppu->LY_register);

    bool new_stat_0R = (bit_read(ppu->STAT_register, 6) && (ppu->LY_register == ppu->LYC_register)) || (bit_read(ppu->STAT_register, 5) && ((ppu->currMode == MODE_2) || (ppu->LY_register == 144))) ||
                       (bit_read(ppu->STAT_register, 4) && (ppu->currMode == MODE_1)) || (bit_read(ppu->STAT_register, 3) && (ppu->currMode == MODE_0));

    if (!ppu->stat_OR && new_stat_0R)
        bit_set(&IF_register, 1);

    if (ppu->triggerVBLANKintr) {
        ppu->triggerVBLANKintr = false;
        bit_set(&IF_register, 0);
    }

    ppu->stat_OR = new_stat_0R;
}

u8 oam_read(u16 addr) {
    // TODO DMA blocking
    return oam.memory[addr - OAMstartingAddr];
}

void oam_write(u16 addr, u8 data) { oam.memory[addr - OAMstartingAddr] = data; }

u8 read_ppu(u16 addr) {
    switch (addr) {
        case 0xFF40:
            return _ppu.LCDC_register;
        case 0xFF41:
            return _ppu.STAT_register;
        case 0xFF42:
            return _ppu.SCY_register;
        case 0xFF43:
            return _ppu.SCX_register;
        case 0xFF44:
            return _ppu.LY_register;
        case 0xFF45:
            return _ppu.LYC_register;
        case 0xFF46:
            return oam.DMA_CTR_REGISTER;
        case 0xFF47:
            return _ppu.BGP_register;
        case 0xFF48:
            return _ppu.OBP0_register;
        case 0xFF49:
            return _ppu.OBP1_register;
        case 0xFF4A:
            return _ppu.WY_register;
        case 0xFF4B:
            return _ppu.WX_register;
        default:
            printInvalidAddr(addr);
            return 0;
    }
}

void write_ppu(u16 addr, u8 data) {
    switch (addr) {
        case 0xFF40:
            _ppu.LCDC_register = data;
            break;
        case 0xFF41:
            _ppu.STAT_register = (data & 0xFC) | (_ppu.STAT_register & 0x83);
            break;
        case 0xFF42:
            _ppu.SCY_register = data;
            break;
        case 0xFF43:
            _ppu.SCX_register = data;
            break;
        case 0xFF44:
            _ppu.LY_register = data;
            break;
        case 0xFF45:
            _ppu.LYC_register = data;
            break;
        case 0xFF46:
            DMA_writeREG(&oam, data);
            break;
        case 0xFF47:
            _ppu.BGP_register = data;
            break;
        case 0xFF48:
            _ppu.OBP0_register = data;
            break;
        case 0xFF49:
            _ppu.OBP1_register = data;
            break;
        case 0xFF4A:
            _ppu.WY_register = data;
            break;
        case 0xFF4B:
            _ppu.WX_register = data;
            break;
        default:
            printInvalidAddr(addr);
    }
}

void ppu_init() {
    _ppu.LY_register = 0x00;
    _ppu.X_position = 0;
    _ppu.SCY_register = 0x00;
    _ppu.SCX_register = 0x00;
    _ppu.WX_register = 0x00;
    _ppu.WY_register = 0x00;
    _ppu.LCDC_register = 0x91;
    _ppu.LYC_register = 0x00;
    _ppu.STAT_register = 0x85;
    _ppu.BGP_register = 0xFC;
    _ppu.spriteBuffer.numStoredSprites = 0;
    _ppu.scanLineTicks = 0;
    _ppu.currMode = MODE_2;
    _ppu.isSecondCycle = false;
    _ppu.MODE2addr = OAMstartingAddr;
    _ppu.WY_equal_LY = false;
    _ppu.stat_OR = false;
    _ppu.firstTimeInScanline = true;
    _ppu.triggerVBLANKintr = false;

    FIFO_reset(&backgroundFIFO);
    FIFO_reset(&spriteFIFO);

    _pixelFetcher.X_position = 0;
    _pixelFetcher.WINDOW_LINE_COUNTER = 0;
    _pixelFetcher.currFetching = BACKGROUND;
    pixelFetcher_setState(&_pixelFetcher, fetchTileNo);
    _pixelFetcher.spriteToFetch.isCurrentlyFetching = false;
    _pixelFetcher.firstTimeInScanline = true;
    _pixelFetcher.incrWINDOW = false;
    _pixelFetcher.isSecondCycle = false;
    for (int i = 0; i < 10; i++)
        _pixelFetcher.spriteToFetch.alreadyAsked[i] = false;

    _pixelMixer.waitNumCycles = 0;
    _pixelMixer.state = STALLED;

    oam.state = INACTIVE;
}

void ppu_tick() {
    DMA_tick(&oam);
    // if turned off do nothing
    if (!bit_read(_ppu.LCDC_register, 7)) {
        _ppu.LY_register = 0x00;
        _ppu.currMode = MODE_0;
        bit_clear(&_ppu.STAT_register, 0);
        bit_clear(&_ppu.STAT_register, 1);
        return;
    }

    trigger_intr(&_ppu);

    // TODO update mode of STAT only when the mode changes
    switch (_ppu.currMode) {
        case MODE_2: // OAM Scan - 80 TCycles
            bit_clear(&_ppu.STAT_register, 0);
            bit_set(&_ppu.STAT_register, 1);
            ppu_MODE2_tick(&_ppu);
            break;
        case MODE_3: // Drawing
            bit_set(&_ppu.STAT_register, 0);
            bit_set(&_ppu.STAT_register, 1);
            ppu_MODE3_tick(&_ppu, &_pixelMixer, &_pixelFetcher, &backgroundFIFO, &spriteFIFO);
            break;
        case MODE_0: // HBlank - pad scanline to 456 TCycles
            bit_clear(&_ppu.STAT_register, 0);
            bit_clear(&_ppu.STAT_register, 1);
            ppu_MODE0_tick(&_ppu, &_pixelFetcher, &_pixelMixer, &backgroundFIFO, &spriteFIFO);
            break;
        case MODE_1: // VBlank 10x456 = 4560 TCycles
            bit_set(&_ppu.STAT_register, 0);
            bit_clear(&_ppu.STAT_register, 1);
            ppu_MODE1_tick(&_ppu, &_pixelFetcher);
            break;
    }
}
