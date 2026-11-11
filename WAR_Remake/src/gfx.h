/*
 * The Ancient Art of War - remake
 * gfx.h: the 320x200 indexed-colour game screen.
 */
#ifndef WAR_GFX_H
#define WAR_GFX_H

#include "assets.h"

typedef struct {
    uint8_t px[SCREEN_W * SCREEN_H];   /* palette indexes */
    RGB     pal[16];
} Screen;

void screen_clear(Screen *s, uint8_t color);
void screen_set_palette(Screen *s, const RGB *pal, int n);
void screen_blit(Screen *s, const Image *img, int x, int y);
void screen_fill(Screen *s, int x, int y, int w, int h, uint8_t color);

/* Draw text with the game's 8x8 font. bg < 0 = transparent background. */
void screen_text(Screen *s, const GameAssets *ga, int x, int y, const char *text, uint8_t fg, int bg);

/* Draw an 8x12 map sprite the way WAR.EXE does: a white figure with a black
 * outline (left and right of each figure pixel), or a black figure with a
 * white outline. Only rows clip_top <= y < clip_bottom are touched. */
void screen_sprite(Screen *s, const uint8_t spr[SPRITE_H][SPRITE_W], int x, int y, bool white_figure,
                   uint8_t white, uint8_t black, int clip_top, int clip_bottom);

/* Index of the palette entry closest to (r,g,b). */
uint8_t screen_nearest(const Screen *s, int ncolors, uint8_t r, uint8_t g, uint8_t b);

/* Convert to 32-bit XRGB8888 pixels (SCREEN_W * SCREEN_H). */
void screen_to_xrgb(const Screen *s, uint32_t *out);

#endif
