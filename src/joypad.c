#include "cpu.h"
#include "types.h"

#include <SDL2/SDL.h>

// CONTROLS
static const unsigned char UP_KEY = SDL_SCANCODE_UP;
static const unsigned char DOWN_KEY = SDL_SCANCODE_DOWN;
static const unsigned char RIGHT_KEY = SDL_SCANCODE_RIGHT;
static const unsigned char LEFT_KEY = SDL_SCANCODE_LEFT;

static const unsigned char A_KEY = SDL_SCANCODE_A;
static const unsigned char B_KEY = SDL_SCANCODE_W;
static const unsigned char START_KEY = SDL_SCANCODE_D;
static const unsigned char SELECT_KEY = SDL_SCANCODE_SPACE;

static u8 joypad;

const unsigned char *keyboardArr = NULL;

u8 joypad_read() {
    // when both dpad and Ssab are disabled, the lower nible is 0xF
    u8 val = ((joypad & 0x30) == 0x30) ? joypad | 0xF : joypad;
    return val;
}

void joypad_write(u8 data) { joypad = (data & 0xF0) | (joypad & 0x0F); }

void joypad_init() { joypad = 0xCF; }

void joypad_readInput() {
    if (keyboardArr == NULL)
        return;

    bool DPAD_SELECTED = !bit_read(joypad, 4);
    bool SS_SELECTED = !bit_read(joypad, 5);
    // default no keys pressed
    bool right = false, left = false, up = false, down = false;

    // filter inputs to prevent impossible combinations(TOP - DOWN / RIGHT - LEFT)
    if (keyboardArr[RIGHT_KEY] == 1)
        right = true;
    else if (keyboardArr[LEFT_KEY] == 1)
        left = true;

    if (keyboardArr[UP_KEY] == 1)
        up = true;
    else if (keyboardArr[DOWN_KEY] == 1)
        down = true;

    u8 oldJoypad = joypad;

    // BIT0 - A | Right
    if ((DPAD_SELECTED && right) || (SS_SELECTED && (keyboardArr[A_KEY] == 1)))
        bit_clear(&joypad, 0);
    else
        bit_set(&joypad, 0);

    // BIT1 - B | Left
    if ((DPAD_SELECTED && left) || (SS_SELECTED && (keyboardArr[B_KEY] == 1)))
        bit_clear(&joypad, 1);
    else
        bit_set(&joypad, 1);

    // BIT2 - Up | Select
    if ((DPAD_SELECTED && up) || (SS_SELECTED && (keyboardArr[START_KEY] == 1)))
        bit_clear(&joypad, 2);
    else
        bit_set(&joypad, 2);

    // BIT3 - Start | Down
    if ((DPAD_SELECTED && down) || (SS_SELECTED && (keyboardArr[SELECT_KEY] == 1)))
        bit_clear(&joypad, 3);
    else
        bit_set(&joypad, 3);

    // interrupt on falling edge
    if ((((~joypad) & 0x0F) & (oldJoypad & 0x0F)) != 0)
        bit_set(&IF_register, 4);
}
