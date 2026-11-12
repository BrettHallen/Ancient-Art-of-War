/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * savegame.c: carrying on a game saved by the original program.
 *
 * save_game (ovl1:35C4) writes 217Eh bytes of WAR.EXE's data segment, from
 * DS:8868 to A9E5, and load_saved_game (ovl1:37E5) reads them back over the
 * same place before campaign_play carries on. Everything the strategic game
 * keeps between iterations lives there: the squad and flag tables, the path
 * memory, the enemy commander's maps and route memory, the counters, the
 * opponent's values and the rules. The remake's Play fields are named after
 * those DS addresses; this file copies each one across.
 *
 * Not in the file: the random number state (DS:0535), and AAE3-AAEF (the
 * food tick, the cursor-on-a-squad flag, the last sighting region). WAR.EXE
 * carries on with whatever those held; the remake starts them afresh.
 */
#include "savegame.h"

#include <SDL3/SDL.h>

#define AT(addr) ((addr) - ORIGINAL_S_BASE)

static int w(const uint8_t *s, unsigned addr)          /* a signed word at a DS address */
{
    return (int16_t)(s[AT(addr)] | (s[AT(addr) + 1] << 8));
}

static int wi(const uint8_t *s, unsigned addr, int i) { return w(s, addr + 2u * (unsigned)i); }

int savegame_campaign_number(const uint8_t *s) { return w(s, 0xA694); }
const uint8_t *savegame_map(const uint8_t *s) { return s + AT(0x9D82); }

bool savegame_import(Play *p, Campaign *c, const GameAssets *ga, const uint8_t *s, size_t size, uint32_t seed)
{
    if (!s || size != ORIGINAL_S_SIZE) return false;

    /* the campaign: the map, the armies as they stand (the same 400 bytes
     * as a T file's, DS:9E60), the opponent and the rules */
    SDL_memset(c, 0, sizeof *c);
    SDL_memcpy(c->map, s + AT(0x9D82), sizeof c->map);
    SDL_memcpy(c->armies, s + AT(0x9E60), sizeof c->armies);
    SDL_memset(c->story_raw, ' ', sizeof c->story_raw);   /* DS:9782 is the path memory in play */
    c->opponent = w(s, 0xA67C) & 7;
    for (int r = 0; r < RULE_COUNT; r++) c->rules[r] = (uint16_t)wi(s, 0xA705, r);
    SDL_strlcpy(c->file, "S", sizeof c->file);
    SDL_strlcpy(c->title, "Saved game", sizeof c->title);
    campaign_refresh(c);
    for (int row = 0; row < MAP_H; row++)
        for (int col = 0; col < MAP_W; col++)
            if (c->map[row][col] < 1 || c->map[row][col] > NUM_TILES) return false;   /* not a saved game */
    c->loaded = true;

    play_prepare(p, c, ga, seed);

    /* the squads (DS:9E60-A1F7, 20 entries each) */
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        int slot = wi(s, 0x9EB0, i);
        q->used = slot != 9999;
        q->moving = q->used ? slot : 0;
        q->friendly = wi(s, 0x9E60, i) == -1;
        q->speed = wi(s, 0x9E88, i);
        q->x = wi(s, 0x9ED8, i);
        q->y = wi(s, 0x9F00, i);
        q->formation = s[AT(0x9F28) + i];
        q->condition = s[AT(0x9F3C) + i];
        q->food = s[AT(0x9F50) + i];
        q->archers = s[AT(0x9FA0) + i];
        q->knights = s[AT(0x9FB4) + i];
        q->barbarians = s[AT(0x9FC8) + i];
        q->spies = s[AT(0x9FDC) + i];
        q->men = q->archers + q->knights + q->barbarians;
        int fight = wi(s, 0xA0F4, i);                  /* the foe + 1, or 0 */
        q->fighting = fight > 0 && fight <= MAX_SQUADS;
        q->foe = q->fighting ? fight - 1 : 0;
        int clock = wi(s, 0xA004, i);                  /* -2 none, 1.. counting, then the side (-1 or 0) */
        if (!q->fighting) q->clock = 0;
        else if (clock >= 1 && clock < p->fight_delay) q->clock = clock;
        else q->clock = FIGHT_ON;
        q->kill = wi(s, 0xA1E4, i);
        q->seen = wi(s, 0xA11C, i) != 0;
        q->countdown = wi(s, 0xA0CC, i);
        q->delay = wi(s, 0xA8D9, i);
        q->has_path = wi(s, 0xA194, i) != 0;
        q->path_pos = SDL_clamp(wi(s, 0xA144, i), 0, PATH_SIZE - 1);
        q->on_route = wi(s, 0xA1BC, i);
        q->route_pos = SDL_clamp(wi(s, 0xA16C, i), 0, MAP_W * MAP_H - 1);
        q->planned = wi(s, 0xA0B8, i) != 0;
        q->dest = wi(s, 0xA02C, i);
        int mark = wi(s, 0xA51E, i) & 0xFFFF;          /* the mark's screen-buffer address, or 9999 */
        q->marked = mark != 9999 && mark >= 0xB68;
        if (q->marked) {
            q->mark_x = (mark - 0xB68) % 80;
            q->mark_y = (mark - 0xB68) / 80;
        }
    }
    for (int i = 0; i < MAX_FLAGS; i++) {               /* DS:9F64, 9F78, 9F8C */
        p->flag[i].x = wi(s, 0x9F64, i);
        p->flag[i].y = wi(s, 0x9F78, i);
        p->flag[i].present = p->flag[i].x >= 0;
        p->flag[i].white = wi(s, 0x9F8C, i) == -1;
    }

    /* the path memory (DS:9782) and the enemy's route memory (9B6A) */
    SDL_memcpy(p->path, s + AT(0x9782), PATH_SIZE);
    SDL_memcpy(p->route, s + AT(0x9B6A), sizeof p->route);
    p->path_next = SDL_clamp(w(s, 0xA502), 0, PATH_SIZE - 1);

    /* the enemy commander's maps and plans */
    for (int k = 0; k < MAP_W * MAP_H; k++) {
        p->cell_cost[k] = (int16_t)wi(s, 0x8B8C, k);
        p->supply[k] = (int16_t)wi(s, 0x8F74, k);
        p->cell_value[k] = (int16_t)wi(s, 0x9294, k);
    }
    for (int k = 0; k < OPP_PARAMS; k++) p->opp[k] = (int16_t)wi(s, 0xA4DC, k);
    p->ai_sq = w(s, 0xA50A);
    p->home_flag = w(s, 0xA510);
    p->home_cell = w(s, 0xA512);
    p->target_cell = w(s, 0xA514);
    p->attack_flag = w(s, 0xA516);
    p->ai_dist = w(s, 0xA8C7);
    p->ai_next = w(s, 0xA8CD);
    p->ai_dest = w(s, 0xA8CF);
    p->value_fort = w(s, 0xA8D1);
    p->value_village = w(s, 0xA8D3);
    p->value_bridge = w(s, 0xA8D5);
    p->value_flag = w(s, 0xA8D7);

    /* the map view, the cursor, time and the counters */
    p->view_top = SDL_clamp(w(s, 0x886A), 0, VIEW_TOP_MAX);
    p->cx = w(s, 0x9E4A);
    p->cy = w(s, 0x9E4C);
    p->sel = w(s, 0xA506);
    if (p->sel < -1 || p->sel >= MAX_SQUADS) p->sel = -1;
    p->time_div = w(s, 0xA51C);
    if (p->time_div != 1 && p->time_div != 4 && p->time_div != 16) p->time_div = 64;
    p->tick_count = w(s, 0xA51A);
    p->alert_timer = w(s, 0xA518);
    p->trace_toggle = w(s, 0xA8C1) != 0;
    p->result_code = w(s, 0xA6F7);
    p->captured_mine = w(s, 0xA6F9);
    p->captured_theirs = w(s, 0xA6FB);
    p->lost_mine = w(s, 0xA6FD);
    p->lost_theirs = w(s, 0xA6FF);
    p->flags_captured = w(s, 0xA701);
    p->flags_taken = w(s, 0xA703);

    /* a damaged file must not send the game outside its tables */
    p->cx = SDL_clamp(p->cx, 1, 0x4E);
    p->cy = SDL_clamp(p->cy, 2, 0xC3);
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        q->x = SDL_clamp(q->x, 0, 0x4E);
        q->y = SDL_clamp(q->y, 0, 0xC4);
        q->formation = SDL_clamp(q->formation, 1, 9);
        q->condition = SDL_clamp(q->condition, 0, 49);
        q->food = SDL_clamp(q->food, 0, 49);
        q->speed = SDL_clamp(q->speed, 1, 3);
        if (q->moving < 0 || q->moving > 3) q->moving = 0;
        if (q->archers > 14 || q->knights > 14 || q->barbarians > 14 || q->spies > 14) q->used = false;
        if (q->dest < -1 || q->dest >= MAP_W * MAP_H) q->dest = -1;
        if (q->fighting && (q->foe == i || !p->sq[q->foe].used)) { q->fighting = false; q->clock = 0; }
        q->mark_x = SDL_clamp(q->mark_x, 0, 0x4E);
        q->mark_y = SDL_clamp(q->mark_y, 0, 0xC8);
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        if (p->flag[i].x > 0x4E || p->flag[i].y < 0 || p->flag[i].y > 0xC4) p->flag[i].present = false;
    }
    if (p->home_flag < -1 || p->home_flag >= MAX_FLAGS) p->home_flag = -1;
    if (p->attack_flag < -1 || p->attack_flag >= MAX_FLAGS) p->attack_flag = -1;
    if (p->home_cell < -1 || p->home_cell >= MAP_W * MAP_H) p->home_cell = -1;
    if (p->target_cell < -1 || p->target_cell >= MAP_W * MAP_H) p->target_cell = -1;
    if (p->ai_dest < -1 || p->ai_dest >= MAP_W * MAP_H) p->ai_dest = -1;
    p->ai_sq = SDL_clamp(p->ai_sq, 0, MAX_SQUADS - 1);
    p->ai_next = SDL_clamp(p->ai_next, 0, MAX_SQUADS - 1);
    for (int k = 0; k < OPP_PARAMS; k++) p->opp[k] = (int16_t)SDL_clamp(p->opp[k], -1000, 1000);
    return true;
}
