/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * mapedit.c: step 7, the map maker, following overlay 4 of WAR.EXE.
 *
 * The top of the screen shows four rows of the map; below it are a small
 * plan of the whole map with the cursor's cell, and one page of 18 tiles
 * with the letters that put them down. P turns to the next of the four
 * pages (72 tiles), the arrows move the cursor over the map.
 */
#include "mapedit.h"
#include "play.h"
#include "sound.h"

#include <SDL3/SDL.h>

#define VIEW_ROWS 4           /* map rows shown (ovl4_3A2F: 40 tiles) */
#define PAGE_TILES 18

static const char *O0S(const MapEdit *e, unsigned off)
{
    return assets_buf_string(e->o0, sizeof e->o0, off);
}

void mapedit_start(MapEdit *e, const GameAssets *ga, const Campaign *c, bool new_campaign)
{
    SDL_memset(e, 0, sizeof *e);
    e->ga = ga;
    size_t size = 0;
    uint8_t *d = assets_load_file(ga->dir, "O0", &size);
    if (d) {
        SDL_memcpy(e->o0, d, SDL_min(size, sizeof e->o0));
        e->have_o0 = true;
    }
    SDL_free(d);
    e->new_campaign = new_campaign;
    e->result = MAPEDIT_RUNNING;
    if (new_campaign) {
        /* ovl4:32A4: a fresh map of the two grass tiles, in a checkerboard */
        int v = 1;
        for (int i = 0; i < MAP_W * MAP_H; i++) {
            if (i % 10 == 0 && ++v > 2) v = 1;
            e->map[i / MAP_W][i % MAP_W] = (uint8_t)v;
            if (++v > 2) v = 1;
        }
        e->phase = 0;
    } else {
        SDL_memcpy(e->map, c->map, sizeof e->map);
        e->phase = 1;
    }
}

/* ovl4_35BF: one key while editing */
static void edit_key(MapEdit *e, int k)
{
    if (k == PK_ENTER) {                       /* when done */
        snd_beep();
        e->phase = 2;
        e->wait = 0;
        return;
    }
    switch (k) {
    case PK_UP:                                /* ovl4:35EF */
        if (e->row > 0) e->row--;
        if (e->row < e->top) e->top--;
        return;
    case PK_LEFT:                              /* the end of the row before */
        if (e->col > 0) e->col--;
        else if (e->row > 0) { e->row--; e->col = MAP_W - 1; }
        if (e->row < e->top) e->top--;
        return;
    case PK_RIGHT:                             /* the start of the next row */
        if (e->col < MAP_W - 1) e->col++;
        else if (e->row < MAP_H - 1) { e->row++; e->col = 0; }
        if (e->top + VIEW_ROWS - 1 < e->row) e->top++;
        return;
    case PK_DOWN:
        if (e->row < MAP_H - 1) e->row++;
        if (e->top + VIEW_ROWS - 1 < e->row) e->top++;
        return;
    case PK_P:                                 /* ovl4:371E: the next page of tiles */
        snd_beep();
        e->page = (e->page + 1) % 4;
        return;
    default:
        break;
    }
    /* ovl4:376A: A-S (not P) put down a tile of this page; the table at
     * O0+14Bh lists the letters by scan code */
    static const char LETTERS[] = "Q0ER000IO00000ASDFGHJKL0000000C0BNM";
    if (k < 0x10 || k >= 0x10 + (int)sizeof LETTERS - 1) return;
    int ch = LETTERS[k - 0x10];
    if (ch == '0') return;
    snd_beep();
    if (ch > 'P') ch--;                        /* no P: Q-S are tiles 16-18 */
    e->map[e->row][e->col] = (uint8_t)(ch - 0x40 + PAGE_TILES * e->page);
}

void mapedit_key(MapEdit *e, int k)
{
    if (e->result != MAPEDIT_RUNNING || e->answer_shown) return;
    switch (e->phase) {
    case 0:                                    /* ovl4:3261: Enter to begin, Esc if you've changed your mind */
        if (k == PK_ESC) { snd_beep(); e->result = MAPEDIT_ABORT; }
        else if (k == PK_ENTER) { snd_beep(); e->phase = 1; }
        break;
    case 1:
        edit_key(e, k);
        break;
    case 2:
        if (e->wait < 7) break;                /* delay(7000) before the answer is read */
        if (k == PK_Y) {
            if (e->new_campaign) e->result = MAPEDIT_NEXT;
            else { e->answer_shown = true; e->wait = 0; }
        } else if (k == PK_N) {
            if (e->new_campaign) e->phase = 1;
            else { e->answer_shown = true; e->wait = -1000; }   /* -1000: the answer was No */
        }
        break;
    default:
        break;
    }
}

void mapedit_frame(MapEdit *e)
{
    if (e->phase == 2 && !e->answer_shown) e->wait++;
    if (e->answer_shown) {                     /* "Yes" or "No" shows for a moment */
        bool yes = e->wait >= 0;
        e->wait++;
        if ((yes ? e->wait : e->wait + 1000) > 8) e->result = yes ? MAPEDIT_SAVE : MAPEDIT_DISCARD;
    }
}

/* ------------------------------------------------------------------ drawing */

typedef struct {
    Screen *s;
    const GameAssets *ga;
    uint8_t col[4];           /* CGA colours 0-3 in the screen palette */
    uint8_t inv[16];          /* XOR with 3 (CGA), or the nearest opposite colour */
} Canvas;

static void text(const Canvas *c, int col, int row, const char *str)
{
    screen_text(c->s, c->ga, col * 8, row * 8, str, c->col[3], c->col[0]);
}

static void bitmap(const Canvas *c, const uint8_t *buf, size_t size, unsigned off, int x, int y)
{
    int w = 0, h = 0;
    assets_bitmap_px(buf, size, off, 0, 0, &w, &h);
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++)
            if (x + xx < SCREEN_W && y + yy < SCREEN_H)
                c->s->px[(y + yy) * SCREEN_W + x + xx] = c->col[assets_bitmap_px(buf, size, off, xx, yy, NULL, NULL)];
}

static void tile(const Canvas *c, const AssetSet *set, int t, int x, int y)
{
    if (t < 0 || t >= NUM_TILES || !set->tiles[t].px) return;
    screen_blit(c->s, &set->tiles[t], x, y);
}

void mapedit_draw(const MapEdit *e, Screen *s, GfxMode mode)
{
    const AssetSet *set = &e->ga->set[mode];
    if (!set->loaded) set = &e->ga->set[MODE_CGA];
    int ncolors = set->tiles[0].ncolors;
    screen_set_palette(s, set->tiles[0].pal, ncolors);
    Canvas c = { s, e->ga, { 0 }, { 0 } };
    for (int i = 0; i < 4; i++) c.col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    for (int i = 0; i < 16; i++) {
        if (ncolors <= 4) c.inv[i] = (uint8_t)(i ^ 3);
        else c.inv[i] = (uint8_t)(i ^ 15);   /* WARET.EXE XORs the 4-bit pixels */
    }
    screen_clear(s, c.col[0]);
    /* ovl4:31BD: [A5C0] = 1, text_rows_add: a line over the bottom text row (draw_text_area_border) */
    screen_fill(s, 0, 190, SCREEN_W, 1, c.col[3]);

    /* ovl4_33FA: the panel: a line under the map, a line beside the plan,
     * the letters and one page of tiles */
    char line[41];
    SDL_memset(line, '_', 40);
    line[40] = 0;
    text(&c, 0, 10, line);
    for (int y = 0x58; y < 0x58 + 0x66; y++) {          /* draw_vline: byte 0Fh at byte 10h */
        s->px[y * SCREEN_W + 0x10 * 4 + 2] = c.col[3];
        s->px[y * SCREEN_W + 0x10 * 4 + 3] = c.col[3];
    }
    for (int i = 0; i < PAGE_TILES; i++) {
        char l[2] = { (char)('A' + i + (i >= 15)), 0 };   /* no P */
        text(&c, 12 + (i % 6) * 5, 14 + (i / 6) * 4, l);
        /* ovl4_3558; WARET.EXE on an EGA or VGA draws at whole bytes of 8 pixels: 4 pixels left */
        int tx = (0x15 + (i % 6) * 10) * 4;
        if (gfx_card == CARD_EGA || gfx_card == CARD_VGA) tx &= ~7;
        tile(&c, set, e->page * PAGE_TILES + i, tx, 0x5A + (i / 6) * 0x20);
    }
    /* ovl4_3B79: the plan of the map, 10 x 20 cells of 4 pixels */
    for (int y = 90; y <= 170; y++)
        for (int k = 0; k <= 10; k++) s->px[y * SCREEN_W + 11 + 4 * k] = c.col[1];
    for (int k = 0; k <= 20; k++)
        for (int x = 12; x < 52; x++) s->px[(90 + 4 * k) * SCREEN_W + x] = c.col[1];

    if (e->phase == 0) {                       /* ovl4:31D6: the intro */
        text(&c, 1, 1, O0S(e, 0x48));          /* There are 6 steps to make a campaign. */
        text(&c, 1, 2, O0S(e, 0x6F));          /* The first step is to make a map. */
        text(&c, 1, 5, O0S(e, 0xB1));          /* Press   to begin or    if you've */
        text(&c, 1, 6, O0S(e, 0xD2));          /* changed your mind. */
        bitmap(&c, e->ga->ds, DS_SIZE, 0x424, 0x0D * 4, 0x28);    /* [Enter] */
        bitmap(&c, e->o0, sizeof e->o0, 0xF6, 0x29 * 4, 0x28);    /* [Esc] */
        return;
    }

    /* ovl4_3A2F: four rows of the map, the cursor's tile inverted (3B21) */
    for (int r = 0; r < VIEW_ROWS; r++)
        for (int col = 0; col < MAP_W; col++)
            tile(&c, set, e->map[e->top + r][col] - 1, col * TILE_W, r * TILE_H);
    int cx = e->col * TILE_W, cy = (e->row - e->top) * TILE_H;
    for (int y = cy; y < cy + TILE_H; y++)
        for (int x = cx; x < cx + TILE_W; x++) s->px[y * SCREEN_W + x] = c.inv[s->px[y * SCREEN_W + x] & 15];
    /* ovl4_3B51: the cursor's cell on the plan */
    /* WARET.EXE (DOSBox-X): on a Tandy 1000 the cell is light red; on an EGA or VGA light red is
     * ORed over the cell and the grid lines right of and under it (4 x 5 pixels) */
    if (gfx_card == CARD_EGA || gfx_card == CARD_VGA) {
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 4; x++) s->px[(90 + 4 * e->row + y) * SCREEN_W + 12 + 4 * e->col + x] |= 12;
    } else screen_fill(s, 12 + 4 * e->col, 90 + 4 * e->row, 4, 4, gfx_card == CARD_TANDY ? 12 : c.col[3]);

    char num[16];
    text(&c, 1, 22, O0S(e, 0xE5));            /* PAGE 1 */
    SDL_snprintf(num, sizeof num, "%d", e->page + 1);
    text(&c, 6, 22, num);
    if (e->phase == 2) {
        text(&c, 0, 24, O0S(e, e->new_campaign ? 0x186 : 0x12A));   /* Continue to the next step? / Save the last changes? */
        if (e->answer_shown) text(&c, 32, 24, O0S(e, e->wait >= 0 ? 0x16F : 0x183));   /* Yes / No */
        return;
    }
    /* ovl4_34DB: Page [A] - [S]  [Enter] when done */
    text(&c, 0, 24, O0S(e, 0x17E));
    bitmap(&c, e->o0, sizeof e->o0, 0x00, 0x0C * 4, 0xC0);
    text(&c, 8, 24, "-");
    bitmap(&c, e->o0, sizeof e->o0, 0x24, 0x12 * 4, 0xC0);
    bitmap(&c, e->ga->ds, DS_SIZE, 0x424, 0x35 * 4, 0xC0);
    text(&c, 30, 24, O0S(e, 0xEC));
}
