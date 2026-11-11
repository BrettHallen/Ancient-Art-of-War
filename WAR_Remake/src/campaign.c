/*
 * The Ancient Art of War - remake
 * campaign.c: loading the built-in campaigns and drawing their maps.
 */
#include "campaign.h"

#include <SDL3/SDL.h>

const char *const OPPONENT_NAMES[8] = {
    "Athena", "Alexander the Great", "Geronimo", "Crazy Ivan",
    "Caesar", "Genghis Khan", "Napoleon Bonaparte", "Sun Tzu",
};

static const char *const T_FILES[NUM_CAMPAIGNS] = { "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9", "TA" };

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  rds16(const uint8_t *p) { return (int16_t)rd16(p); }

/* Army block (400 bytes). Word arrays have 20 entries (one per squad) unless
 * noted; byte arrays have 20 entries. Loaded by WAR.EXE to DS:9E60. */
enum {
    ARMY_SIDE      = 0,    /* word: -1 friend, 0 foe */
    ARMY_SPEED     = 40,   /* word: 1 fast, 2 march, 3 slow */
    ARMY_SLOT      = 80,   /* word: 9999 = no squad in this slot */
    ARMY_X         = 120,  /* word: byte column in the map buffer (x 4 pixels) */
    ARMY_Y         = 160,  /* word: line pair (x 2 pixels) */
    ARMY_FORMATION = 200,  /* byte: 1-9 */
    ARMY_CONDITION = 220,  /* byte */
    ARMY_FOOD      = 240,  /* byte */
    ARMY_FLAG_X    = 260,  /* word x 10: -1 = no flag */
    ARMY_FLAG_Y    = 280,  /* word x 10 */
    ARMY_FLAG_SIDE = 300,  /* word x 10: -1 white (friend), 0 black */
    ARMY_ARCHERS   = 320,  /* byte */
    ARMY_KNIGHTS   = 340,  /* byte */
    ARMY_BARBARIANS= 360,  /* byte */
    ARMY_SPIES     = 380,  /* byte */
};

static void decode_armies(Campaign *c)
{
    const uint8_t *a = c->armies;
    c->num_friendly = c->num_enemy = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        Squad *q = &c->squads[i];
        q->present = rd16(a + ARMY_SLOT + 2 * i) != 9999;
        q->friendly = rds16(a + ARMY_SIDE + 2 * i) == -1;
        q->x = rds16(a + ARMY_X + 2 * i) * 4;
        q->y = rds16(a + ARMY_Y + 2 * i) * 2;
        q->speed = rds16(a + ARMY_SPEED + 2 * i);
        q->formation = a[ARMY_FORMATION + i];
        q->condition = a[ARMY_CONDITION + i];
        q->food = a[ARMY_FOOD + i];
        q->archers = a[ARMY_ARCHERS + i];
        q->knights = a[ARMY_KNIGHTS + i];
        q->barbarians = a[ARMY_BARBARIANS + i];
        q->spies = a[ARMY_SPIES + i];
        if (q->present) {
            if (q->friendly) c->num_friendly++;
            else c->num_enemy++;
        }
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        Flag *f = &c->flags[i];
        int x = rds16(a + ARMY_FLAG_X + 2 * i);
        f->present = x >= 0;
        f->x = x * 4;
        f->y = rds16(a + ARMY_FLAG_Y + 2 * i) * 2;
        f->white = rds16(a + ARMY_FLAG_SIDE + 2 * i) == -1;
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

/* TITLES: 12 three-byte file names (T0..TB), then the titles, padded with
 * spaces: "TITLES", one per campaign, then "( data disk )". */
static void load_titles(Campaign *c, const char *dir)
{
    size_t size = 0;
    uint8_t *d = assets_load_file(dir, "TITLES", &size);
    if (!d || size < 36) { SDL_free(d); return; }

    char text[1024];
    clean_text(text, sizeof text, d + 36, size - 36);
    SDL_free(d);

    /* split on runs of 2+ spaces */
    int n = 0;
    char *p = text;
    while (*p && n < NUM_CAMPAIGNS + 1) {
        while (*p == ' ') p++;
        if (!*p) break;
        char *start = p;
        while (*p && !(p[0] == ' ' && p[1] == ' ')) p++;
        char save = *p;
        *p = 0;
        if (n > 0 && n <= NUM_CAMPAIGNS) SDL_strlcpy(c[n - 1].title, start, sizeof c[n - 1].title);
        n++;   /* entry 0 is the heading "TITLES" */
        *p = save;
    }
}

/* ovl1_7870 load_campaign_file: story, map, armies, opponent and rules */
bool campaign_load_file(Campaign *c, const char *dir, const char *name)
{
    SDL_memset(c, 0, sizeof *c);
    SDL_strlcpy(c->file, name, sizeof c->file);
    SDL_snprintf(c->title, sizeof c->title, "Campaign %s", name);
    size_t size = 0;
    uint8_t *d = assets_load_file(dir, name, &size);
    if (!d || size < T_FILE_SIZE) { SDL_free(d); return false; }
    clean_text(c->story, sizeof c->story, d, 840);
    SDL_memcpy(c->story_raw, d, 840);
    SDL_memcpy(c->map, d + 0x348, sizeof c->map);
    SDL_memcpy(c->armies, d + 0x410, sizeof c->armies);
    decode_armies(c);
    c->opponent = rd16(d + 0x5A0) & 7;
    for (int r = 0; r < RULE_COUNT; r++) c->rules[r] = rd16(d + 0x5A2 + 2 * r);
    c->loaded = true;
    SDL_free(d);
    return true;
}

int campaigns_load(Campaign out[NUM_CAMPAIGNS], const char *dir)
{
    int loaded = 0;
    for (int i = 0; i < NUM_CAMPAIGNS; i++)
        if (campaign_load_file(&out[i], dir, T_FILES[i])) loaded++;
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
            if (t < 0 || t >= NUM_TILES) continue;
            /* clip to the map area, so the text rows below stay clear */
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

    /* squads, then flags on top (WAR.EXE rt_0f2c draws them in this order) */
    int ncolors = set->tiles[0].ncolors;
    uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
    uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
    for (int i = 0; i < MAX_SQUADS; i++) {
        const Squad *q = &c->squads[i];
        if (!q->present) continue;
        screen_sprite(s, ga->squad_sprite[POSE_STANDING], q->x, q->y - scroll_px, q->friendly, white, black, 0, MAP_VIEW_H);
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        const Flag *f = &c->flags[i];
        if (!f->present) continue;
        screen_sprite(s, ga->flag_sprite, f->x, f->y - scroll_px, f->white, white, black, 0, MAP_VIEW_H);
    }
}
