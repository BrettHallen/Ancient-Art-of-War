/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/maps_demo.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Screen scr; static uint32_t rgb[SCREEN_W*SCREEN_H];
int main(int argc, char **argv)
{
    char err[256]; (void)argc;
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    for (int c = 3; c <= 10; c++) for (int top = 0; top <= 1; top++) {
        play_start(&play, &camps[c], &ga, 1);
        play.view_top = top ? 0 : VIEW_TOP_MAX;
        play_draw(&play, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
        SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
        char p[256]; SDL_snprintf(p, sizeof p, "%s/%s_%d.bmp", argv[2], camps[c].file, top); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
    }
}
