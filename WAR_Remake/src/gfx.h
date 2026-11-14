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

/* The video card the game is shown on, as the original chose its program and
 * colours: WAR.EXE on a CGA (or a Hercules card with WAR HERC), WARET.EXE on
 * an EGA, a Tandy 1000 or a VGA. WARET.EXE draws the same 16-colour pictures
 * (WARET.DAT) on all three, but only on a VGA does it load each picture's own
 * palette into the DAC (INT 10h AX=1012h, the loader at WARET 0133:01B2 when a
 * VGA was found, [277E]); an EGA or a Tandy 1000 shows them in the standard 16
 * colours. The Tandy version also draws a few things differently (the
 * pointer, the map cursor, the outline of squads and flags); see gfx_card. */
typedef enum { CARD_CGA, CARD_COMPOSITE, CARD_COMPOSITE2, CARD_EGA, CARD_TANDY, CARD_VGA, CARD_HERC, CARD_COUNT } VideoCard;
/* CARD_COMPOSITE, CARD_COMPOSITE2: WAR.EXE on a CGA shown on a TV or composite
 * monitor, the old IBM CGA and the new one (composite.c) */

/* The card the screen is being drawn for (main sets it). The drawing code asks
 * it only where WARET.EXE's EGA/VGA and Tandy versions differ. */
extern VideoCard gfx_card;

const char *card_name(VideoCard c);                    /* "CGA", "CGA composite", "CGA composite (new)", "EGA", "Tandy", "VGA", "Hercules" */
bool card_from_name(const char *s, VideoCard *c);      /* cga, composite, composite2, ega, tandy, vga, herc or hercules (any case) */
GfxMode card_assets(VideoCard c);                      /* the pictures it shows: MODE_VGA for the 16-colour cards */

/* A CGA screen as a TV shows it: 640 x 200 (each pixel is two "hdots", whose
 * colours depend on the pixels either side), old or new CGA (composite.c) */
#define COMP_W 640
void screen_to_composite(const Screen *s, bool new_cga, uint32_t *out);

/* drawn as WARET.EXE draws (an EGA, a Tandy 1000 or a VGA) */
static inline bool card_waret(void) { return gfx_card == CARD_EGA || gfx_card == CARD_TANDY || gfx_card == CARD_VGA; }

/* The standard 16 colours (an EGA's or a Tandy 1000's defaults) */
extern const RGB STD16[16];

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
