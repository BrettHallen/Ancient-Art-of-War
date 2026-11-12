/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_blink.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m;
static void shot(const char *p) { menu_draw(m, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4); SDL_SaveBMP(s, p); SDL_DestroySurface(s); }
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); } }
int main(int argc, char **argv) { (void)argc; char e[256]; assets_load(&ga, argv[1], e, 256); campaigns_load(camps, argv[1]);
    m = menu_create(&ga, camps, argv[2]); menu_title(m); frames(1);
    char line[200] = ""; for (int i = 0; i < 60; i++) { frames(1); menu_draw(m, &scr, MODE_CGA); line[i] = scr.px[60 * SCREEN_W + 74] == 0 ? 'X' : '.'; }
    SDL_Log("%s", line);
    frames(15 - 61 % 28 + 28); shot("blink_inv.bmp");
    menu_key(m, PK_EQUALS); frames(1); shot("blink_after_key.bmp");
    frames(16); shot("blink_inv2.bmp");
    menu_key(m, PK_EQUALS); frames(1); shot("blink_after_key2.bmp");
    return 0; }
