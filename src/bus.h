#ifndef BUS_H
#define BUS_H

#include <stdio.h>
#include <string.h>

#include "cartridge.h"
#include "ppu.h"
#include "timing.h"
#include "types.h"

u8 bus_read(u16 addr, bool tick);
void bus_write(u16 addr, u8 data, bool tick);

#endif
