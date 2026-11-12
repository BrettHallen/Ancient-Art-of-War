/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/pl.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/assets.h"
#include "../src/gfx.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Screen s; static uint32_t rgb[64000];
int main(int c, char **v){ char e[256]; (void)c; assets_load(&ga, v[1], e, 256);
 for (int m=0;m<2;m++){ const Image *im=&ga.set[m].pictures[PIC_FORMATION_EDITOR]; SDL_Log("mode %d %dx%d nc %d", m, im->w, im->h, im->ncolors);
  screen_set_palette(&s, im->pal, im->ncolors); screen_blit(&s, im, 0, 0); screen_to_xrgb(&s, rgb);
  SDL_Surface *su=SDL_CreateSurfaceFrom(320,200,SDL_PIXELFORMAT_XRGB8888,rgb,1280); SDL_SaveBMP(su, m?"pl_vga.bmp":"pl_cga.bmp");} return 0;}
