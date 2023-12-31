#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

#include <SDL2/SDL_surface.h>

extern const int width;
extern const int height;
extern const u16 FPS;
extern const u16 frameTicks;

extern SDL_Surface *surface;

void createSurface();
void pushToScreen(u8 pixel, u8 x, u8 y);

#endif
