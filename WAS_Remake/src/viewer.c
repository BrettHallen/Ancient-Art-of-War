/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * viewer.c: the asset viewer (see viewer.h).
 */
#include "viewer.h"

#include <SDL3/SDL.h>

static const char *const SPRITE_NAMES[MSPR_COUNT] = {
    "anchor", "battle", "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "pennant", "cargo",
};

void viewer_init(Viewer *v, const GameAssets *ga, const Campaign *campaigns)
{
    SDL_memset(v, 0, sizeof *v);
    v->ga = ga;
    v->campaigns = campaigns;
    v->caption = true;
    for (int i = 0; i < NUM_CAMPAIGNS; i++) v->scroll[i] = SCROLL_BOTTOM;   /* start at the bottom, as WAR did */
}

bool viewer_is_map(const Viewer *v) { return v->index >= SCREEN_MAP_FIRST && v->index < NUM_SCREENS; }

void viewer_scroll(Viewer *v, int delta)
{
    if (!viewer_is_map(v) || v->page) return;
    int *sc = &v->scroll[v->index - SCREEN_MAP_FIRST];
    *sc = SDL_clamp(*sc + delta, 0, SCROLL_BOTTOM);
}

void viewer_key(Viewer *v, ViewerKey k)
{
    switch (k) {
    case VK_NEXT:    v->index = (v->index + 1) % NUM_SCREENS; v->page = 0; break;
    case VK_PREV:    v->index = (v->index + NUM_SCREENS - 1) % NUM_SCREENS; v->page = 0; break;
    case VK_FIRST:   v->index = 0; v->page = 0; break;
    case VK_UP:      if (viewer_is_map(v) && !v->page) viewer_scroll(v, -TILE_H); else viewer_key(v, VK_PREV); break;
    case VK_DOWN:    if (viewer_is_map(v) && !v->page) viewer_scroll(v, TILE_H); else viewer_key(v, VK_NEXT); break;
    case VK_TOP:     viewer_scroll(v, -MAP_PIXEL_H); break;
    case VK_BOTTOM:  viewer_scroll(v, MAP_PIXEL_H); break;
    case VK_MODE:                    /* CGA, then 16 colours (if WAST.DAT is there), then Hercules */
        if (v->herc) { v->herc = false; v->mode = MODE_CGA; }
        else if (v->mode == MODE_CGA && v->ga->set[MODE_16].loaded) v->mode = MODE_16;
        else { v->mode = MODE_CGA; v->herc = true; }
        break;
    case VK_CAPTION: v->caption = !v->caption; break;
    case VK_STORY:   if (viewer_is_map(v)) v->page = (v->page + 1) % 3; break;
    default: break;
    }
}

const char *viewer_mode_name(const Viewer *v) { return v->herc ? "Hercules" : mode_name(v->mode); }

const char *viewer_title(const Viewer *v, int index)
{
    if (index == SCREEN_TILES)   return "Map tiles";
    if (index == SCREEN_FONT)    return "Font";
    if (index == SCREEN_SPRITES) return "Map sprites";
    if (index >= SCREEN_MAP_FIRST && index < NUM_SCREENS) return v->campaigns[index - SCREEN_MAP_FIRST].title;
    return picture_name((PictureId)index);
}

/* ------------------------------------------------------------------ the screens */

static void set_text_palette(Screen *s, GfxMode mode)
{
    if (mode == MODE_CGA) screen_set_palette(s, CGA_PALETTE, 4);
    else                  screen_set_palette(s, STD16_PALETTE, 16);
}

static void draw_picture(const Viewer *v, Screen *s)
{
    const Image *img = &v->ga->set[v->mode].pictures[v->index];
    if (!img->px) {
        /* SPP and SPQ are only in the CGA version. Their colours (palette 1:
         * black, cyan, magenta, white) are colours 0, 11, 13 and 15 of the 16. */
        static const uint8_t CGA_TO_16[4] = { 0, 11, 13, 15 };
        const Image *cga = &v->ga->set[MODE_CGA].pictures[v->index];
        screen_set_palette(s, STD16_PALETTE, 16);
        for (int i = 0; i < SCREEN_W * SCREEN_H && cga->px; i++) s->px[i] = CGA_TO_16[cga->px[i] & 3];
        return;
    }
    screen_set_palette(s, img->pal, img->ncolors);
    screen_blit(s, img, 0, 0);
}

static void draw_tiles(const Viewer *v, Screen *s)
{
    const AssetSet *set = &v->ga->set[v->mode];
    screen_set_palette(s, set->tiles[0].pal, set->tiles[0].ncolors);
    screen_clear(s, 0);
    for (int t = 0; t < NUM_TILES; t++)    /* 10 across x 10 down: 94 tiles fill 9.4 rows */
        screen_blit(s, &set->tiles[t], (t % 10) * TILE_W, (t / 10) * TILE_H);
}

static void draw_font(const Viewer *v, Screen *s)
{
    set_text_palette(s, v->mode);
    screen_clear(s, 0);
    uint8_t white = v->mode == MODE_CGA ? 3 : 15;
    char line[17];
    for (int row = 0; row < 6; row++) {
        int n = 0;
        for (int c = 0; c < 16; c++) {
            int ch = FONT_FIRST + row * 16 + c;
            line[n++] = (ch < FONT_FIRST + FONT_COUNT) ? (char)ch : ' ';
        }
        line[n] = 0;
        screen_text(s, v->ga, 32, 40 + row * 16, line, white, -1);
    }
    screen_text(s, v->ga, 32, 16, "The Ancient Art of War at Sea font", white, -1);
}

/* Each sprite four times bigger, white with a black outline over a sea tile
 * and black with a white outline, as the two sides' ships are drawn. */
static void draw_sprites(const Viewer *v, Screen *s)
{
    set_text_palette(s, v->mode);
    uint8_t white = v->mode == MODE_CGA ? 3 : 15, black = 0;
    uint8_t grey = v->mode == MODE_CGA ? 1 : 9;
    screen_clear(s, 0);
    screen_text(s, v->ga, 8, 4, "Map sprites (WAS.EXE DS:049B-05CA)", white, -1);
    for (int k = 0; k < MSPR_COUNT; k++) {
        const Sprite *sp = &v->ga->map_sprite[k];
        int cx = 8 + (k % 10) * 31, cy = 22 + (k / 10) * 84;
        for (int side = 0; side < 2; side++) {
            Screen tmp;
            SDL_memset(tmp.px, grey, sizeof tmp.px);   /* a plain background shows the outline */
            screen_sprite(&tmp, sp, 0, 0, side == 0, white, black, 0, SCREEN_H);
            for (int y = 0; y < sp->h; y++)
                for (int x = 0; x < sp->w; x++)
                    screen_fill(s, cx + x * 3, cy + side * 30 + y * 3, 3, 3, tmp.px[y * SCREEN_W + x]);
        }
        char name[16];
        SDL_strlcpy(name, SPRITE_NAMES[k], sizeof name);
        name[3] = 0;   /* fits under the sprite */
        screen_text(s, v->ga, cx, cy + 60, name, white, -1);
    }
}

static void draw_map(const Viewer *v, Screen *s)
{
    const Campaign *c = &v->campaigns[v->index - SCREEN_MAP_FIRST];
    int ncolors = v->ga->set[v->mode].tiles[0].ncolors;
    campaign_draw_map(s, v->ga, v->mode, c, v->scroll[v->index - SCREEN_MAP_FIRST]);
    uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
    uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
    screen_fill(s, 0, MAP_VIEW_H, SCREEN_W, SCREEN_H - MAP_VIEW_H, black);
    char line[64];
    if (c->loaded) SDL_snprintf(line, sizeof line, "%s", c->title);
    else           SDL_snprintf(line, sizeof line, "%s not found", c->file);
    line[40] = 0;
    screen_text(s, v->ga, 0, 184, line, white, black);
    if (c->loaded) {
        SDL_snprintf(line, sizeof line, "vs %s", OPPONENT_NAMES[c->opponent]);
        line[30] = 0;
        screen_text(s, v->ga, 0, 192, line, white, black);
        SDL_snprintf(line, sizeof line, "%2d v %2d", c->ships_friendly, c->ships_enemy);
        screen_text(s, v->ga, SCREEN_W - 7 * 8, 192, line, white, black);
    }
}

/* The story as it is stored (26 rows of 32 characters; the blank rows left out) */
static void draw_story(const Viewer *v, Screen *s)
{
    const Campaign *c = &v->campaigns[v->index - SCREEN_MAP_FIRST];
    set_text_palette(s, v->mode);
    screen_clear(s, 0);
    uint8_t white = v->mode == MODE_CGA ? 3 : 15;
    if (!c->loaded) { screen_text(s, v->ga, 0, 0, "Not found", white, -1); return; }
    int row = 0;
    for (int r = 0; r < 26 && row < 24; r++) {
        char line[33];
        int n = 0;
        bool blank = true;
        for (int i = 0; i < 32; i++) {
            uint8_t ch = c->story_raw[r * 32 + i];
            if (ch == 0x01) continue;   /* the marker after a capital */
            line[n] = (ch >= 0x20 && ch < 0x7F) ? (char)ch : ' ';
            if (line[n++] != ' ') blank = false;
        }
        line[n] = 0;
        if (!blank) screen_text(s, v->ga, 32, row++ * 8, line, white, -1);
    }
}

/* The squadrons and their ships, the player's on the left and the enemy's on
 * the right; L line, F frigate, * flagship (Info's LINE, FRIGATE, FLAG) */
static void draw_ships(const Viewer *v, Screen *s)
{
    const Campaign *c = &v->campaigns[v->index - SCREEN_MAP_FIRST];
    set_text_palette(s, v->mode);
    screen_clear(s, 0);
    uint8_t white = v->mode == MODE_CGA ? 3 : 15, hi = v->mode == MODE_CGA ? 1 : 11;
    char line[48];
    SDL_snprintf(line, sizeof line, "Friend: %d/%d", c->num_friendly, c->ships_friendly);
    screen_text(s, v->ga, 0, 0, line, hi, -1);
    SDL_snprintf(line, sizeof line, "Foe: %d/%d", c->num_enemy, c->ships_enemy);
    screen_text(s, v->ga, 160, 0, line, hi, -1);
    int row[2] = { 0, 0 };
    for (int q = 0; q < MAX_SHIPS; q++) {
        if (!c->squadrons[q].present) continue;
        int side = c->squadrons[q].friendly ? 0 : 1;
        for (int i = 0; i < MAX_SHIPS; i++) {
            const Ship *sh = &c->ships[i];
            if (!sh->present || sh->squadron != q) continue;
            int y = 10 + row[side] * 8;
            char t = sh->type == SHIP_LINE ? 'L' : sh->type == SHIP_FRIGATE ? 'F' : '*';
            SDL_snprintf(line, sizeof line, "%c%c%s", t, ' ', sh->name);
            if (y <= SCREEN_H - 16) screen_text(s, v->ga, side * 160, y, line, white, -1);
            row[side]++;
        }
    }
    if (row[0] > 23 || row[1] > 23)
        screen_text(s, v->ga, 0, SCREEN_H - 16, "(more ships than fit)", hi, -1);
}

void viewer_draw(const Viewer *v, Screen *s)
{
    if (v->index < PIC_COUNT)            draw_picture(v, s);
    else if (v->index == SCREEN_TILES)   draw_tiles(v, s);
    else if (v->index == SCREEN_FONT)    draw_font(v, s);
    else if (v->index == SCREEN_SPRITES) draw_sprites(v, s);
    else if (v->page == 1)               draw_story(v, s);
    else if (v->page == 2)               draw_ships(v, s);
    else                                 draw_map(v, s);

    if (v->caption && !(viewer_is_map(v) && !v->page)) {   /* maps use the bottom lines themselves */
        int ncolors = s->pal[15].r || s->pal[15].g || s->pal[15].b ? 16 : 4;
        uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
        uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
        char cap[96];
        const char *note = "";
        if (v->index < PIC_COUNT && v->mode == MODE_16 && !v->ga->set[MODE_16].pictures[v->index].px)
            note = " (CGA only)";
        SDL_snprintf(cap, sizeof cap, "%s %d/%d: %s%s", viewer_mode_name(v), v->index + 1, NUM_SCREENS,
                     viewer_title(v, v->index), note);
        cap[40] = 0;   /* 40 characters per line */
        screen_fill(s, 0, SCREEN_H - 8, SCREEN_W, 8, black);
        screen_text(s, v->ga, 0, SCREEN_H - 8, cap, white, black);
    }
}
