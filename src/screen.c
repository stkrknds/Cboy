#include "screen.h"

const int width = 160;
const int height = 144;
const u16 FPS = 60;
const u16 frameTicks = 1000 / FPS;

// taken from https://blog.tigris.fr/2019/10/30/writing-an-emulator-the-first-real-pixel/
const SDL_Color colors[4] = {
    {0xe0, 0xf0, 0xe7, 0xff}, // White
    {0x8b, 0xa3, 0x94, 0xff}, // Light gray
    {0x55, 0x64, 0x5a, 0xff}, // Dark gray
    {0x34, 0x3d, 0x37, 0xff}, // Black
};

SDL_Surface *surface;

void createSurface() {
    surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
    // set gameboy palette
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 4);
}

// surface must be locked before calling this function
void pushToScreen(u8 pixel, u8 x, u8 y) { ((u8 *)surface->pixels)[y * surface->pitch + x] = pixel; }
