/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * campaign.c: the built-in campaigns S0-SA (see campaign.h).
 */
#include "campaign.h"

#include <SDL3/SDL.h>

/* In the order of their descriptions in SO2 and their portraits in SPJ */
const char *const OPPONENT_NAMES[NUM_OPPONENTS] = {
    "Thor Foote", "Duke of Medina Sidonia", "Blackbeard", "Martin Tromp", "John Paul Jones", "Lord Nelson",
};

static const char *const S_FILES[NUM_CAMPAIGNS] = {
    "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "SA",
};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  rds16(const uint8_t *p) { return (int16_t)rd16(p); }

/* Squadron and ship block (480 bytes, DS:5CDD), 40 entries per field.
 * Squadrons: words +0..+319 and bytes +320. Ships: bytes +360..+479. */
enum {
    SQ_SIDE     = 0,     /* word: -1 friend, 0 foe (draw_squadrons rt:10D2: 0 is hidden unless seen) */
    SQ_SLOT     = 80,    /* word: 9999 = empty slot; in play the squadron's speed (0 = anchored) */
    SQ_X        = 160,   /* word: byte column in the map buffer (x 4 pixels) */
    SQ_Y        = 240,   /* word: line pair (x 2 pixels) */
    SQ_SUPPLIES = 320,   /* byte: the Info bar (rt:0A85) */
    SHIP_SQUAD  = 360,   /* byte: the ship's squadron, 255 = none */
    SHIP_SAILS  = 400,   /* byte: sails damage (Info: 5 - damage, rt:09FB) */
    SHIP_TYPE   = 440,   /* byte: 1 LINE, 2 FRIGATE, else FLAG (rt:09BF) */
};

static void decode_ships(Campaign *c, const uint8_t *names)
{
    const uint8_t *a = c->ships_raw;
    c->num_friendly = c->num_enemy = c->ships_friendly = c->ships_enemy = 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        Squadron *q = &c->squadrons[i];
        q->present = rd16(a + SQ_SLOT + 2 * i) != 9999;
        q->friendly = rds16(a + SQ_SIDE + 2 * i) != 0;
        q->x = rds16(a + SQ_X + 2 * i);
        q->y = rds16(a + SQ_Y + 2 * i);
        q->supplies = a[SQ_SUPPLIES + i];
        q->flagship = false;
        q->ships = 0;
        if (q->present) {
            if (q->friendly) c->num_friendly++;
            else c->num_enemy++;
        }
    }
    for (int i = 0; i < MAX_SHIPS; i++) {
        Ship *s = &c->ships[i];
        s->squadron = a[SHIP_SQUAD + i];
        s->present = s->squadron < MAX_SHIPS && c->squadrons[s->squadron].present;
        s->sails_damage = a[SHIP_SAILS + i];
        s->hull_damage = c->hull_raw[i];
        s->type = a[SHIP_TYPE + i];
        size_t n = 0;
        for (; n < SHIP_NAME_LEN && names[i * SHIP_NAME_LEN + n]; n++) {
            uint8_t ch = names[i * SHIP_NAME_LEN + n];
            s->name[n] = (ch >= 0x20 && ch < 0x7F) ? (char)ch : ' ';
        }
        s->name[n] = 0;
        if (!s->present) continue;
        Squadron *q = &c->squadrons[s->squadron];
        q->ships++;
        if (s->type == SHIP_FLAG) q->flagship = true;   /* rt_0688 sets DS:5EBD to 1 */
        if (q->friendly) c->ships_friendly++;
        else c->ships_enemy++;
    }
}

/* Copy text, dropping the 0x01 capital markers and trailing spaces. */
static void clean_text(char *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    size_t o = 0;
    for (size_t i = 0; i < inlen && o + 1 < outlen; i++) {
        if (in[i] == 0x01) continue;
        if (in[i] == 0) break;
        out[o++] = (in[i] >= 0x20 && in[i] < 0x7F) ? (char)in[i] : ' ';
    }
    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;
}

/* SITLES: 12 three-byte file names (S0..SB), then 26 rows of 32 characters:
 * "TITLES", one title on every other row, then "( data disk )". Only the
 * first 876 bytes are the list (the file is padded to 1024). */
static void load_titles(Campaign *c, const char *dir)
{
    size_t size = 0;
    uint8_t *d = assets_load_file(dir, "SITLES", &size);
    if (!d || size < 36 + 32) { SDL_free(d); return; }
    size_t text_len = SDL_min(size, (size_t)876) - 36;
    int n = 0;
    for (size_t row = 0; (row + 1) * 32 <= text_len && n <= NUM_CAMPAIGNS; row++) {
        char line[64];
        clean_text(line, sizeof line, d + 36 + row * 32, 32);
        const char *p = line;
        while (*p == ' ') p++;
        if (!*p) continue;
        if (n > 0) SDL_strlcpy(c[n - 1].title, p, sizeof c[n - 1].title);   /* entry 0 is "TITLES" */
        n++;
    }
    SDL_free(d);
}

/* ovl1:C388 load_campaign */
bool campaign_load_file(Campaign *c, const char *dir, const char *name)
{
    SDL_memset(c, 0, sizeof *c);
    SDL_strlcpy(c->file, name, sizeof c->file);
    SDL_snprintf(c->title, sizeof c->title, "Campaign %s", name);
    size_t size = 0;
    uint8_t *d = assets_load_file(dir, name, &size);
    if (!d || size < S_FILE_MIN) { SDL_free(d); return false; }
    clean_text(c->story, sizeof c->story, d, 840);
    SDL_memcpy(c->story_raw, d, 840);
    SDL_memcpy(c->map, d + 0x348, sizeof c->map);
    SDL_memcpy(c->ships_raw, d + 0x410, sizeof c->ships_raw);
    c->opponent = rd16(d + 0x5F0);
    if (c->opponent < 0 || c->opponent >= NUM_OPPONENTS) c->opponent = 0;
    for (int r = 0; r < NUM_RULES; r++) c->rules[r] = rd16(d + 0x5F2 + 2 * r);
    if (size >= S_FILE_MIN + MAX_SHIPS) SDL_memcpy(c->hull_raw, d + S_FILE_MIN, MAX_SHIPS);
    decode_ships(c, d + 0x604);
    c->loaded = true;
    SDL_free(d);
    return true;
}

int campaigns_load(Campaign out[NUM_CAMPAIGNS], const char *dir)
{
    int loaded = 0;
    for (int i = 0; i < NUM_CAMPAIGNS; i++)
        if (campaign_load_file(&out[i], dir, S_FILES[i])) loaded++;
    load_titles(out, dir);
    return loaded;
}

void campaign_draw_tiles(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px, int view_h)
{
    const AssetSet *set = &ga->set[mode];
    screen_set_palette(s, set->tiles[0].pal, set->tiles[0].ncolors);
    view_h = SDL_clamp(view_h, 0, SCREEN_H);
    screen_fill(s, 0, 0, SCREEN_W, view_h, 0);
    scroll_px = SDL_clamp(scroll_px, 0, MAP_PIXEL_H - view_h);
    int first = scroll_px / TILE_H, offset = scroll_px % TILE_H;
    for (int row = first; row < MAP_H && (row - first) * TILE_H - offset < view_h; row++) {
        for (int col = 0; col < MAP_W; col++) {
            int t = c->map[row][col] - 1;
            if (t < 0 || t >= NUM_TILES || !set->tiles[t].px) continue;
            const Image *tile = &set->tiles[t];
            int y0 = (row - first) * TILE_H - offset;
            for (int y = 0; y < TILE_H; y++) {
                int sy = y0 + y;
                if (sy < 0 || sy >= view_h) continue;
                SDL_memcpy(&s->px[sy * SCREEN_W + col * TILE_W], &tile->px[y * TILE_W], TILE_W);
            }
        }
    }
}

void campaign_draw_map(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px)
{
    const AssetSet *set = &ga->set[mode];
    scroll_px = SDL_clamp(scroll_px, 0, MAP_PIXEL_H - MAP_VIEW_H);
    campaign_draw_tiles(s, ga, mode, c, scroll_px, MAP_VIEW_H);
    int ncolors = set->tiles[0].ncolors;
    uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
    uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
    /* draw_squadrons (rt:10D2): the squadron, then the pennant one line pair above */
    for (int i = 0; i < MAX_SHIPS; i++) {
        const Squadron *q = &c->squadrons[i];
        if (!q->present) continue;
        /* the map address is (y + 1) * 80 + x in the map buffer (ovl1:C4D8) */
        int px = q->x * 4, py = (q->y + 1) * 2 - scroll_px;
        screen_sprite(s, &ga->map_sprite[MSPR_ANCHOR], px, py, q->friendly, white, black, 0, MAP_VIEW_H);
        if (q->flagship)
            screen_sprite(s, &ga->map_sprite[MSPR_PENNANT], px, py - 2, q->friendly, white, black, 0, MAP_VIEW_H);
    }
}
