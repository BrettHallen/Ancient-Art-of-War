/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * formed.c: step 7, "Teach squads new formations", following overlay 5 of
 * WAR.EXE.
 *
 * The 14 men of a formation stand on the field of picture PL, with the
 * numbers of each kind in a box at the top left and the formation's plan
 * (A, B and K) at the bottom left. - and + choose the formation. For 6-9,
 * M brings a pointer: over a man, M picks him up, the arrows walk him
 * (with O8's own walking scripts), A, B and K make him an archer,
 * barbarian or knight, and M puts him down. S saves the formations to F.
 */
#include "formed.h"
#include "play.h"
#include "sound.h"

#include <SDL3/SDL.h>

/* O8 (overlay 5 loads it at DS:0B68 + 3E80h) */
static const unsigned SPRITE[7] = { 0, 0x000, 0x0AC, 0x120, 0x1CC, 0x240, 0x2EC };   /* DS:A933.. */
static const unsigned STAND[3] = { 0x398, 0x416, 0x494 };   /* A0CC: knight, barbarian, archer */
#define HIT_OFFSETS 0x5EB                                    /* [A929]: 8 offsets of the pointer on a man */
#define SCRIPT_END 1000                                      /* steps above this end a script (1010) */

/* ovl5_357B: the walking script for a direction key, per kind of man */
static unsigned walk_script(int key, int type)
{
    static const struct { int key; unsigned off[3]; } W[] = {
        { PK_HOME,  { 0x3C2, 0x440, 0x4BE } },   /* A144 */
        { PK_UP,    { 0x3FA, 0x478, 0x4F6 } },   /* A901 */
        { PK_PGUP,  { 0x3D0, 0x44E, 0x4CC } },   /* A194 */
        { PK_LEFT,  { 0x3A6, 0x424, 0x4A2 } },   /* A0F4 */
        { PK_RIGHT, { 0x3B4, 0x432, 0x4B0 } },   /* A11C */
        { PK_END,   { 0x3DE, 0x45C, 0x4DA } },   /* A1BC */
        { PK_DOWN,  { 0x408, 0x486, 0x504 } },   /* A16C */
        { PK_PGDN,  { 0x3EC, 0x46A, 0x4E8 } },   /* A1E4 */
    };
    for (size_t i = 0; i < SDL_arraysize(W); i++)
        if (W[i].key == key) return W[i].off[type];
    return STAND[type];
}

static int rd(const FormEdit *e, unsigned off)
{
    if (off + 1 >= sizeof e->o8) return SCRIPT_END + 10;
    return (int16_t)(e->o8[off] | (e->o8[off + 1] << 8));
}

static const char *O8S(const FormEdit *e, unsigned off) { return assets_buf_string(e->o8, sizeof e->o8, off); }

static uint8_t *F_type(FormEdit *e) { return e->f + e->form * 14; }        /* [A8BB] */
static uint8_t *F_x(FormEdit *e)    { return e->f + 140 + e->form * 14; }  /* [A8BD] */
static uint8_t *F_y(FormEdit *e)    { return e->f + 280 + e->form * 14; }  /* [A8BF] */

/* ovl5_38EE: one pass of sorting the men by y (the order they're drawn in) */
static void sort_pass(FormEdit *e)
{
    for (int k = 0; k < FORMED_MEN - 1; k++) {
        if (e->y[k] <= e->y[k + 1]) continue;
#define SWAP(a) do { int t_ = a[k]; a[k] = a[k + 1]; a[k + 1] = t_; } while (0)
        SWAP(e->type); SWAP(e->x); SWAP(e->y); SWAP(e->slot); SWAP(e->script); SWAP(e->sprite);
#undef SWAP
        if (e->sel == k) e->sel = k + 1;
        else if (e->sel == k + 1) e->sel = k;
    }
}

/* ovl5_446E: the men back into F */
static void store(FormEdit *e)
{
    for (int i = 0; i < FORMED_MEN; i++) {
        int s = e->slot[i];
        F_type(e)[s] = (uint8_t)e->type[i];
        F_x(e)[s] = (uint8_t)e->x[i];
        F_y(e)[s] = (uint8_t)e->y[i];
    }
}

/* ovl5_4218: the men of formation [A8D1], standing */
static void load(FormEdit *e)
{
    for (int i = 0; i < FORMED_MEN; i++) {
        e->type[i] = SDL_clamp(F_type(e)[i], 0, 2);
        e->x[i] = F_x(e)[i];
        e->y[i] = F_y(e)[i];
        e->slot[i] = i;
        e->script[i] = (int)STAND[e->type[i]];
        e->sprite[i] = 2 + 2 * e->type[i];
    }
    for (int k = 0; k < 10; k++) sort_pass(e);
}

void formed_sort(uint8_t f[420])
{
    for (int fm = 0; fm <= 9; fm++)
        for (int pass = 0; pass < 14; pass++)
            for (int k = fm * 14; k < fm * 14 + 13; k++) {
                if (f[140 + k] <= f[140 + k + 1]) continue;
                for (int t = 0; t < 3; t++) {
                    uint8_t v = f[t * 140 + k];
                    f[t * 140 + k] = f[t * 140 + k + 1];
                    f[t * 140 + k + 1] = v;
                }
            }
}

void formed_start(FormEdit *e, const GameAssets *ga, const uint8_t formations[420])
{
    SDL_memset(e, 0, sizeof *e);
    e->ga = ga;
    size_t size = 0;
    uint8_t *d = assets_load_file(ga->dir, "O8", &size);
    if (d) {
        SDL_memcpy(e->o8, d, SDL_min(size, sizeof e->o8));
        e->have_o8 = true;
    }
    SDL_free(d);
    SDL_memcpy(e->f, formations, sizeof e->f);
    e->form = 1;
    e->mode = 1;
    e->sel = 6;
    e->cx = 0x2A;
    e->cy = 0x91;
    load(e);
}

bool formed_wants_held(const FormEdit *e) { return e->mode == 2 || e->mode == 3; }

/* ovl5_357B: each man takes the next step of his script */
static void step_men(FormEdit *e)
{
    for (int i = 0; i < FORMED_MEN; i++) {
        if (rd(e, (unsigned)e->script[i]) > SCRIPT_END) {          /* the script is over: the next */
            if (e->mode == 3 && i == e->sel) e->script[i] = (int)walk_script(e->walk, e->type[i]);
            else e->script[i] = (int)STAND[e->type[i]];
        }
        unsigned s = (unsigned)e->script[i];
        int d = rd(e, s), dx = d > 10 ? d - 160 : d < -10 ? d + 160 : d;
        if (d < -10 && e->y[i] < 0x1B) d += 160;                    /* the edges of the field */
        else if (d > 10 && e->y[i] > 0x4B) d -= 160;
        if ((dx < 0 && e->x[i] < 0x2D) || (dx > 0 && e->x[i] > 0x49)) d -= dx;
        if (d < -10) { e->y[i] -= 2; e->x[i] += d + 160; }
        else if (d > 10) { e->y[i] += 2; e->x[i] += d - 160; }
        else e->x[i] += d;
        int spr = rd(e, s + 2);
        if (spr >= 1 && spr <= 6) e->sprite[i] = spr;
        e->script[i] = (int)s + 6;
    }
    e->walk = 0;
    sort_pass(e);
}

/* the pointer is on a man (ovl5:3DDE): 1 + the last one, or 0 */
static int man_under_pointer(const FormEdit *e)
{
    int found = 0;
    for (int i = 0; i < FORMED_MEN; i++)
        for (int k = 0; k < 8; k++) {
            int off = rd(e, HIT_OFFSETS + 2u * (unsigned)k);
            if ((e->y[i] + 100) * 80 + e->x[i] + off == e->cy * 80 + e->cx) { found = i + 1; break; }
        }
    return found;
}

void formed_key(FormEdit *e, int k)
{
    if (e->done || !k || e->saving) return;
    if (e->ask == 1) {                                 /* Save the last changes? (Y or N) */
        if (k == PK_Y) { e->ask = 2; e->answer_yes = true; e->wait = 8; }
        else if (k == PK_N) e->done = true;
        return;
    }
    if (e->ask) return;
    bool down = k == PK_EQUALS || k == PK_KP_PLUS, up = k == PK_MINUS || k == PK_KP_MINUS;
    switch (e->mode) {
    case 1:                                            /* ovl5:3B6C */
        if (k == PK_ENTER) {
            snd_beep();
            if (e->changed) e->ask = 1;
            else e->done = true;
        } else if (k == PK_S && e->form > 5) {         /* Save: "Saving Formations" */
            snd_beep();
            store(e);
            e->save = true;
            e->saving = 4;
            e->changed = false;
        } else if (down || up) {
            snd_beep();
            store(e);
            e->form = down ? (e->form % 9) + 1 : (e->form + 7) % 9 + 1;
            load(e);
        } else if (k == PK_M && e->form > 5) {         /* the pointer (ovl5_344B) */
            snd_beep();
            e->mode = 2;
            e->changed = true;
            e->over = e->over_was = man_under_pointer(e);
        }
        break;
    case 2:                                            /* ovl5:3CF2 */
        if (k == PK_E) { snd_beep(); e->mode = 1; }
        else if (k == PK_M && e->over) {               /* pick the man up (ovl5_34BD) */
            snd_beep();
            e->sel = e->over - 1;
            e->mode = 3;
            e->changed = true;
        }
        break;
    case 3:                                            /* ovl5:3E57 */
        if (k == PK_M) {                               /* put him down */
            store(e);
            e->over = e->over_was = 0;
            snd_beep();
            e->mode = 2;
            e->over = e->over_was = man_under_pointer(e);
        } else if (k == PK_B || k == PK_A || k == PK_K) {
            snd_beep();
            int i = e->sel, t = k == PK_B ? 1 : k == PK_A ? 2 : 0;
            if (t == 2 && e->type[i] != 2) e->x[i] -= 2;          /* an archer stands 2 bytes left */
            else if (t != 2 && e->type[i] == 2) e->x[i] += 2;
            e->type[i] = t;
            e->script[i] = (int)STAND[t];
            e->sprite[i] = 2 + 2 * t;
            store(e);
        }
        break;
    default:
        break;
    }
}

void formed_frame(FormEdit *e, int held)
{
    if (e->done) return;
    e->tick++;
    if (e->ask == 2 && --e->wait <= 0) { store(e); e->save = true; e->done = true; return; }
    if (e->saving) e->saving--;
    if (e->mode == 2 && held >= PK_HOME && held <= PK_PGDN) {   /* ovl5:3D44: the pointer */
        static const int8_t DX[11] = { -1, 0, 1, 0, -1, 0, 1, 0, -1, 0, 1 };   /* DS:04F2 */
        static const int8_t DY[11] = { -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1 };   /* DS:04FD */
        e->cx += DX[held - PK_HOME];
        e->cy += DY[held - PK_HOME];
        e->cx = SDL_clamp(e->cx, 0x2A, 0x4E);
        e->cy = SDL_clamp(e->cy, 0x7F, 0xB8);
    }
    if (e->mode == 2) {
        e->over = man_under_pointer(e);
        if (e->over != e->over_was) { snd_buzz(); e->over_was = e->over; }
        return;
    }
    if (held) e->walk = held;
    if (e->tick % 2 == 0) step_men(e);             /* about 10 steps a second */
}

/* ------------------------------------------------------------------ drawing */

static void sprite(const FormEdit *e, Screen *s, unsigned off, int xb, int line, const uint8_t col[4])
{
    int ww = rd(e, off), hh = rd(e, off + 2);
    if (ww <= 0 || ww > 4 || hh <= 0 || hh > 20) return;
    for (int y = 0; y < hh * 2; y++) {
        int sy = line + y;
        if (sy < 50 || sy >= 180) continue;                     /* ovl5_46C5 shows lines 50-179 */
        const uint8_t *row = e->o8 + off + 4 + y * ww * 2;
        for (int x = 0; x < ww * 8; x++) {
            int sx = xb * 4 + x;
            if (sx < 144 || sx >= SCREEN_W) continue;
            int v = (row[x / 4] >> (6 - 2 * (x & 3))) & 3, in = x & 7;
            int l = in > 0 ? (row[(x - 1) / 4] >> (6 - 2 * ((x - 1) & 3))) & 3 : 0;
            int r = in < 7 ? (row[(x + 1) / 4] >> (6 - 2 * ((x + 1) & 3))) & 3 : 0;
            if (v) s->px[sy * SCREEN_W + sx] = col[v];
            else if (l | r) s->px[sy * SCREEN_W + sx] = col[0];
        }
    }
}

static void text(Screen *s, const GameAssets *ga, int col, int row, const char *str, const uint8_t c[4])
{
    screen_text(s, ga, col * 8, row * 8, str, c[3], c[0]);
}

static void ds_bitmap(Screen *s, const GameAssets *ga, unsigned off, int x, int y, const uint8_t c[4])
{
    int w = 0, h = 0;
    assets_ds_bitmap_px(ga, off, 0, 0, &w, &h);
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++)
            if (x + xx < SCREEN_W && y + yy < SCREEN_H)
                s->px[(y + yy) * SCREEN_W + x + xx] = c[assets_ds_bitmap_px(ga, off, xx, yy, NULL, NULL)];
}

void formed_draw(const FormEdit *e, Screen *s, GfxMode mode)
{
    const AssetSet *set = &e->ga->set[mode];
    if (!set->loaded) set = &e->ga->set[MODE_CGA];
    const Image *pl = &set->pictures[PIC_FORMATION_EDITOR];
    screen_set_palette(s, pl->pal, pl->ncolors);
    screen_blit(s, pl, 0, 0);
    uint8_t c[4];
    for (int i = 0; i < 4; i++) c[i] = screen_nearest(s, pl->ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);

    for (int i = 0; i < FORMED_MEN; i++)              /* the men on the field */
        sprite(e, s, SPRITE[SDL_clamp(e->sprite[i], 1, 6)], e->x[i], e->y[i] * 2, c);

    screen_fill(s, 0, 182, SCREEN_W, 1, c[3]);         /* draw_text_area_border */
    screen_fill(s, 0, 183, SCREEN_W, SCREEN_H - 183, c[0]);

    /* ovl5_43D9: the box of numbers */
    for (int y = 0; y < 50; y++)
        for (int x = 0; x < 132; x++) s->px[y * SCREEN_W + x] = c[x < 2 ? 3 : 0];
    screen_fill(s, 0x21 * 4 + 2, 0, 2, 50, c[3]);
    screen_fill(s, 0, 0, 0x22 * 4, 1, c[3]);
    screen_fill(s, 0, 50, 0x22 * 4, 1, c[3]);
    int n[3] = { 0, 0, 0 };
    for (int i = 0; i < FORMED_MEN; i++) n[SDL_clamp(e->type[i], 0, 2)]++;
    char num[16];
    text(s, e->ga, 1, 1, O8S(e, 0x5B5), c);           /* Formation: */
    SDL_snprintf(num, sizeof num, "%d", e->form);
    text(s, e->ga, 12, 1, num, c);
    static const unsigned LABEL[3] = { 0x5C1, 0x5CF, 0x5DD };   /* Knights ----, Barbarians -, Archers ---- */
    for (int t = 0; t < 3; t++) {
        text(s, e->ga, 1, 3 + t, O8S(e, LABEL[t]), c);
        SDL_snprintf(num, sizeof num, "%-2d", n[t]);
        text(s, e->ga, 14, 3 + t, num, c);
    }

    /* formation_preview (rt:0D3F): the plan of the formation, from the men */
    for (int y = 86; y < 190; y++)
        for (int x = 0; x < 68; x++) s->px[y * SCREEN_W + x] = c[x < 2 ? 3 : 0];
    screen_fill(s, 17 * 4 + 2, 86, 2, 104, c[3]);
    screen_fill(s, 0, 86, 72, 1, c[3]);
    screen_fill(s, 0, 190, 72, 1, c[3]);
    for (int i = 0; i < FORMED_MEN; i++) {
        int t = SDL_clamp(e->type[i], 0, 2);
        int xb = SDL_clamp(((e->x[i] + (t == 2 ? 2 : 0)) >> 1) - 22, 1, 15);
        int line = SDL_clamp(e->y[i] + 16, 44, 92) * 2;
        unsigned off = t == 0 ? 0x3DC : t == 1 ? 0x3D0 : 0x3C4;   /* K, B, A */
        for (int r = 0; r < 6; r++) {
            int word = (e->ga->ds[off + r * 2] << 8) | e->ga->ds[off + r * 2 + 1];
            for (int b = 0; b < 8; b++)
                if (line + r < SCREEN_H) s->px[(line + r) * SCREEN_W + xb * 4 + b] = c[(word >> (14 - 2 * b)) & 3];
        }
    }

    if (e->mode == 2) {
        /* ovl5_474A: the pointer's place on the plan */
        int fx = SDL_clamp(e->cx / 2 - 0x16, 1, 15), fy = SDL_clamp(e->cy - 0x56, 0x2C, 0x5C) * 2;
        for (int r = 0; r < 4; r++)
            for (int b = 0; b < 4; b++)
                if ((r & 1) || b == 1 || b == 2) s->px[(fy + r) * SCREEN_W + fx * 4 + b] = c[2];
        /* draw_cursor: the map's cross */
        int x = e->cx * 4, l = (e->cy - 100) * 2;
        static const int arm[6] = { 0, 1, 2, 7, 8, 9 };
        for (int k = 0; k < 6; k++)
            for (int i = 0; i < 4; i++) s->px[(l + arm[k]) * SCREEN_W + x + i] = c[i < 2 ? 0 : 3];
        static const int8_t bar[12] = { 0, 0, 3, 3, 3, -1, -1, 0, 0, 3, 3, 3 };
        for (int k = 4; k <= 5; k++)
            for (int i = 0; i < 12; i++)
                if (bar[i] >= 0 && x - 4 + i < SCREEN_W) s->px[(l + k) * SCREEN_W + x - 4 + i] = c[bar[i]];
    }

    /* rows 23 and 24 */
    if (e->ask) {
        text(s, e->ga, 0, 24, O8S(e, 0x582), c);      /* Save the last changes? (Y or N) */
        if (e->ask == 2) text(s, e->ga, 32, 24, O8S(e, 0x609), c);
        return;
    }
    if (e->saving) { text(s, e->ga, 11, 24, O8S(e, 0x5A3), c); return; }   /* Saving Formations */
    switch (e->mode) {
    case 1:                                            /* ovl5_33CC */
        if (e->form > 5) text(s, e->ga, 0, 24, O8S(e, 0x550), c);   /* Move Save */
        ds_bitmap(s, e->ga, 0x448, 0x14 * 4, 0xC0, c);              /* [+] */
        ds_bitmap(s, e->ga, 0x46C, 0x19 * 4, 0xC0, c);              /* [-] */
        ds_bitmap(s, e->ga, 0x424, 0x3A * 4, 0xC0, c);              /* [Enter] */
        text(s, e->ga, 32, 24, O8S(e, 0x55A), c);                   /* if done */
        break;
    case 2:                                            /* ovl5_344B, 347D */
        text(s, e->ga, 10, 23, O8S(e, 0x512), c);     /* Put pointer over a man to move */
        text(s, e->ga, 0, 24, O8S(e, 0x604), c);      /* End */
        if (e->over) text(s, e->ga, 4, 24, O8S(e, 0x562), c);       /* Mark */
        break;
    case 3: {                                          /* ovl5_34BD, 34F6 */
        text(s, e->ga, 10, 23, O8S(e, 0x531), c);     /* Move the man, press [M] to mark */
        ds_bitmap(s, e->ga, 0x490, 0x3C * 4, 0xB8, c);
        char line[64];
        int t = e->type[SDL_clamp(e->sel, 0, FORMED_MEN - 1)];
        SDL_snprintf(line, sizeof line, "%s %s%s%s", O8S(e, 0x562), t != 2 ? O8S(e, 0x567) : "",
                     t != 1 ? O8S(e, 0x56F) : "", t != 0 ? O8S(e, 0x57A) : "");
        text(s, e->ga, 0, 24, line, c);
        break;
    }
    default:
        break;
    }
}
