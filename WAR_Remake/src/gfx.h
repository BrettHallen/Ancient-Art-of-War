/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
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

/* Hercules (WAR HERC; ovl1_8441): the card is set to 720x348 graphics (CRTC
 * table DS:095E, mode 8Ah: page 1 at B800h, CGA's segment) and the row table
 * DS:096A points each of the 200 CGA rows at a Hercules scanline, from line
 * 72 and byte 4 (four banks of 2000h, 90 bytes a row). WAR.EXE draws exactly
 * as for CGA, so each CGA byte (4 pixels of 2 bits) shows as 8 mono pixels:
 * colour 0 two dark pixels, 1 dark-lit, 2 lit-dark, 3 two lit; the palette
 * makes no difference. The CGA screen s (palette indexes 0-3) becomes
 * HERC_W x HERC_H XRGB8888, lit pixels in DOSBox's grey. */
#define HERC_W 720
#define HERC_H 348
#define HERC_X 32
#define HERC_Y 72
#define HERC_LIT 0xAAAAAAu
void screen_to_herc(const Screen *s, uint32_t *out);

#endif
