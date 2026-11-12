/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gfx.c: the 320x200 indexed-colour game screen.
 */
#include "gfx.h"

#include <SDL3/SDL.h>

void screen_clear(Screen *s, uint8_t color)
{
    SDL_memset(s->px, color, sizeof s->px);
}

void screen_set_palette(Screen *s, const RGB *pal, int n)
{
    SDL_memset(s->pal, 0, sizeof s->pal);
    for (int i = 0; i < n && i < 16; i++) s->pal[i] = pal[i];
}

void screen_blit(Screen *s, const Image *img, int x, int y)
{
    if (!img || !img->px) return;
    for (int j = 0; j < img->h; j++) {
        int sy = y + j;
        if (sy < 0 || sy >= SCREEN_H) continue;
        for (int i = 0; i < img->w; i++) {
            int sx = x + i;
            if (sx >= 0 && sx < SCREEN_W) s->px[sy * SCREEN_W + sx] = img->px[j * img->w + i];
        }
    }
}

void screen_fill(Screen *s, int x, int y, int w, int h, uint8_t color)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            if (i >= 0 && i < SCREEN_W && j >= 0 && j < SCREEN_H) s->px[j * SCREEN_W + i] = color;
}

void screen_text(Screen *s, const GameAssets *ga, int x, int y, const char *text, uint8_t fg, int bg)
{
    for (; *text; text++, x += 8) {
        int c = (unsigned char)*text - FONT_FIRST;
        if (c < 0 || c >= FONT_COUNT) c = '?' - FONT_FIRST;
        for (int r = 0; r < 8; r++) {
            uint8_t bits = ga->font[c][r];
            for (int b = 0; b < 8; b++) {
                int px = x + b, py = y + r;
                if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
                if (bits & (0x80 >> b)) s->px[py * SCREEN_W + px] = fg;
                else if (bg >= 0)       s->px[py * SCREEN_W + px] = (uint8_t)bg;
            }
        }
    }
}

void screen_sprite(Screen *s, const Sprite *spr, int x, int y, bool white_figure,
                   uint8_t white, uint8_t black, int clip_top, int clip_bottom)
{
    /* WAS.EXE (rt_2941): for each byte, white figure = (old & ~(f >> 2) & ~(f << 2)) | f,
     * black figure = (old | (f >> 2) | (f << 2)) & ~f; the shifts stay inside the byte.
     * x must be a multiple of 4 for the bytes to line up, as on the map. */
    uint8_t figure = white_figure ? white : black, outline = white_figure ? black : white;
    for (int j = 0; j < spr->h; j++) {
        int sy = y + j;
        if (sy < clip_top || sy >= clip_bottom || sy < 0 || sy >= SCREEN_H) continue;
        for (int i = 0; i < spr->w; i++) {
            int sx = x + i;
            if (sx < 0 || sx >= SCREEN_W) continue;
            int b = i & 3, base = i - b;
            bool here = spr->px[j][i] != 0;
            bool next = (b > 0 && spr->px[j][base + b - 1]) || (b < 3 && spr->px[j][base + b + 1]);
            if (here)      s->px[sy * SCREEN_W + sx] = figure;
            else if (next) s->px[sy * SCREEN_W + sx] = outline;
        }
    }
}

uint8_t screen_nearest(const Screen *s, int ncolors, uint8_t r, uint8_t g, uint8_t b)
{
    int best = 0;
    long bestd = -1;
    for (int i = 0; i < ncolors && i < 16; i++) {
        long dr = s->pal[i].r - r, dg = s->pal[i].g - g, db = s->pal[i].b - b;
        long d = dr * dr + dg * dg + db * db;
        if (bestd < 0 || d < bestd) { bestd = d; best = i; }
    }
    return (uint8_t)best;
}

void screen_to_xrgb(const Screen *s, uint32_t *out)
{
    uint32_t lut[16];
    for (int i = 0; i < 16; i++)
        lut[i] = 0xFF000000u | ((uint32_t)s->pal[i].r << 16) | ((uint32_t)s->pal[i].g << 8) | s->pal[i].b;
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) out[i] = lut[s->px[i] & 15];
}

void screen_to_herc(const Screen *s, uint32_t *out)
{
    const uint32_t lit = 0xAAAAAAu;          /* as DOSBox shows a Hercules card's pixels */
    for (int i = 0; i < HERC_W * HERC_H; i++) out[i] = 0;
    for (int y = 0; y < SCREEN_H; y++) {
        uint32_t *row = out + (HERC_Y + y) * HERC_W + HERC_X;
        for (int x = 0; x < SCREEN_W; x++) {
            int c = s->px[y * SCREEN_W + x] & 3;
            row[2 * x] = (c & 2) ? lit : 0;
            row[2 * x + 1] = (c & 1) ? lit : 0;
        }
    }
}
