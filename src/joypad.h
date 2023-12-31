#ifndef JOYPAD_H
#define JOYPAD_H

#include "types.h"

u8 joypad_read();
void joypad_write(u8 data);
void joypad_init();
void joypad_readInput();

extern const unsigned char *keyboardArr;

#endif
