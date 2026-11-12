/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/pics.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/assets.h"
#include "../src/gfx.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Screen scr; static uint32_t rgb[SCREEN_W*SCREEN_H];
int main(int c, char **v){ (void)c; char e[256]; if(!assets_load(&ga,v[2],e,256)) return 1;
 const char *names[]={"PM","PI","PE","PF","PJ","PG","PH","PK","PN","PO","PL"};
 for(int m=0;m<2;m++) for(int p=0;p<11;p++){ const Image *im=&ga.set[m].pictures[p]; screen_set_palette(&scr, im->pal, im->ncolors); screen_blit(&scr, im, 0, 0); screen_to_xrgb(&scr, rgb);
  SDL_Surface *s=SDL_CreateSurfaceFrom(SCREEN_W,SCREEN_H,SDL_PIXELFORMAT_XRGB8888,rgb,SCREEN_W*4); char path[256]; SDL_snprintf(path,256,"%s/%s_%s.bmp",v[1],names[p],m?"vga":"cga"); SDL_SaveBMP(s,path); SDL_DestroySurface(s);} }
