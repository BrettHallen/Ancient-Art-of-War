/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gfx.h: the 320x200 indexed-colour game screen.
 */
#ifndef WAS_GFX_H
#define WAS_GFX_H

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

/* Draw a map sprite the way WAS.EXE does (rt_2941, draw_map_sprite): a
 * white figure with a black outline, or a black figure with a white outline.
 * The outline is the pixel left and right of each figure pixel within the
 * same byte (4 pixels) of the CGA buffer. Only rows clip_top <= y <
 * clip_bottom are touched. */
void screen_sprite(Screen *s, const Sprite *spr, int x, int y, bool white_figure,
                   uint8_t white, uint8_t black, int clip_top, int clip_bottom);

/* Index of the palette entry closest to (r,g,b). */
uint8_t screen_nearest(const Screen *s, int ncolors, uint8_t r, uint8_t g, uint8_t b);

/* Convert to 32-bit XRGB8888 pixels (SCREEN_W * SCREEN_H). */
void screen_to_xrgb(const Screen *s, uint32_t *out);

/* Hercules (WAS HERC, ovl1:CE78): the card is set to 720x348 graphics and the
 * row table DS:0D3B points each of the 200 CGA rows at a Hercules scanline
 * (from line 72, byte 4: four banks of 2000h, 90 bytes a row). WAS.EXE draws
 * exactly as for CGA, so each CGA byte (4 pixels of 2 bits) shows as 8 mono
 * pixels: colour 0 as two dark pixels, 1 dark-lit, 2 lit-dark, 3 two lit.
 * The CGA screen s (palette indexes 0-3) becomes HERC_W x HERC_H XRGB8888. */
/* The PCjr (WAS.EXE with [0814] set: the BIOS equipment word's bit 8, no DMA):
 * in the CGA 4-colour mode its palette registers 0-3 choose any of the 16
 * colours for the four pixel values. rt_0c8f (each screen's colours) sets
 * them: with the Options Shoppe's display "rgb" ([684D] 2) always 0, 2, 12,
 * 15; with "tv/composite" (1) the screen's first four, with "black/white" (0)
 * its other four. WAS.EXE's set_cga_palette (BIOS INT 10h AH=0Bh) still
 * runs for the scrolls and the battle's flashes; on a PCjr the BIOS then sets
 * registers 1-3 to 2, 4, 6 (palette 0) or 3, 5, 15 (palette 1), as DOSBox-X
 * shows (14 Nov). */
typedef struct { uint8_t reg[4]; } JrPal;
void jr_screen(JrPal *j, int display, const uint8_t set[8]);   /* rt_0c8f */
void jr_bios(JrPal *j, int pal);                               /* INT 10h AH=0Bh BH=1 */
void screen_apply_jr(Screen *s, const JrPal *j);               /* colours 0-3 as the registers say */

/* WAS.EXE's screens' colour sets for rt_0c8f (composite, then black/white) */
extern const uint8_t JR_STARTUP[8], JR_SCROLL[8], JR_CAMPAIGN_MENU[8], JR_EDITOR_MENU[8], JR_TITLE[8],
                     JR_OPPONENT[8], JR_REPORT[8], JR_REPORT_2[8], JR_OPTIONS[8], JR_EDITORS[8], JR_PLAY[8];

#define HERC_W 720
#define HERC_H 348
#define HERC_X 32
#define HERC_Y 72
void screen_to_herc(const Screen *s, uint32_t *out);

#endif
