#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu.h"
#include "screen.h"

#ifdef DEBUG
void run_frame(SDL_Surface *surface, FILE *logFile) {
#else
void run_frame(SDL_Surface *surface) {
#endif
    SDL_LockSurface(surface);

    // if the ppu is already in VBLANK, run until it isn't
    while (_ppu.currMode == MODE_1)
#ifdef DEBUG
        cpu_run(logFile);
#else
        cpu_run();
#endif

    // once the ppu has entered VBLANK, we can draw the frame
    while (_ppu.currMode != MODE_1) {
#ifdef DEBUG
        cpu_run(logFile);
#else
        cpu_run();
#endif
    }
    SDL_UnlockSurface(surface);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Invalid argument. \n");
        exit(0);
    }

    uint startTicks, endTicks, delta;
    bool quit = false;
    // SDL
    SDL_Window *window = SDL_CreateWindow("Cboy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160, 144, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    createSurface();
    SDL_Texture *texture;
    SDL_Event e;

    SDL_SetWindowResizable(window, SDL_TRUE);
#ifdef DEBUG
    FILE *logFile;
    logFile = fopen("log", "w");
#endif

    // init
    SDL_Init(SDL_INIT_VIDEO);
    cartridge_load(argv[1]);
    cpu_init();
    ppu_init();

    // get keyboard array
    keyboardArr = SDL_GetKeyboardState(NULL);

    // main loop
    while (!quit) {
        startTicks = SDL_GetTicks();

#ifdef DEBUG
        run_frame(surface, logFile);
#else
        run_frame(surface);
#endif
        // TODO remove this
        texture = SDL_CreateTextureFromSurface(renderer, surface);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
            }
        }

        endTicks = SDL_GetTicks();

        delta = endTicks - startTicks;

        if (delta < frameTicks)
            SDL_Delay(frameTicks - delta);

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(texture);
    }

    cartridge_free();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef DEBUG
    fclose(logFile);
#endif

    return 0;
}
