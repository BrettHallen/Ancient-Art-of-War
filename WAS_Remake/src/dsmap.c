/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * dsmap.c: campaign play's state laid out as in WAS.EXE's data segment, and
 * read back from it. The tests run WAS.EXE's own routines on this image
 * (under an emulator) and compare the result with the remake's; the saved
 * game SS (step 6) is the same bytes.
 */
#include "dsmap.h"
#include "play_int.h"

#include <SDL3/SDL.h>

#include <string.h>

static void w16(uint8_t *ds, int a, int v) { ds[a & 0xFFFF] = (uint8_t)v; ds[(a + 1) & 0xFFFF] = (uint8_t)(v >> 8); }
static int  r16(const uint8_t *ds, int a) { return (int16_t)(ds[a & 0xFFFF] | (ds[(a + 1) & 0xFFFF] << 8)); }
static void w8(uint8_t *ds, int a, int v) { ds[a & 0xFFFF] = (uint8_t)v; }
static int  r8(const uint8_t *ds, int a) { return ds[a & 0xFFFF]; }
static int  r8s(const uint8_t *ds, int a) { return (int8_t)ds[a & 0xFFFF]; }

/* the per-squadron word arrays */
#define SQW(F) X(0x5CDD, side) X(0x5D7D, x) X(0x5DCD, y) X(0x5EE5, ctrl) X(0x5FD5, heading) \
    X(0x56FF, partner) X(0x579F, path_pos) X(0x57EF, route_pos) X(0x56AF, countdown) \
    X(0x64E1, move_delay) X(0x6316, dest) X(0x5F35, merchant_dest)

void play_to_ds(const Play *p, uint8_t *ds)
{
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
#define X(a, f) w16(ds, a + 2 * i, q->f);
        SQW(F)
#undef X
        w16(ds, 0x5D2D + 2 * i, q->used ? q->speed : NO_DEST);
        w8(ds, 0x5E1D + i, q->supplies);
        w8(ds, 0x5EBD + i, q->mark);
        w16(ds, 0x574F + 2 * i, q->seen);
        w16(ds, 0x583F + 2 * i, q->has_path);
        w16(ds, 0x588F + 2 * i, q->has_route);
        w8(ds, 0x6581 + i, q->port_speed);
        w8(ds, 0x5687 + i, q->routed);
        w16(ds, 0x5F85 + 2 * i, (q->y + 1) * 80 + q->x);
        const PShip *s = &p->ship[i];
        w8(ds, 0x5E45 + i, s->sq);
        w8(ds, 0x5E6D + i, s->sails);
        w8(ds, 0x6025 + i, s->hull);
        w8(ds, 0x5E95 + i, s->type);
    }
    for (int k = 0; k < 3; k++) w8(ds, 0x6456 + k, p->list[k]);
    w16(ds, 0x62FB, p->sel);
    w16(ds, 0x62FD, p->free_slot);
    w16(ds, 0x688C, p->over_squad);
    w16(ds, 0x5CC7, p->cx);
    w16(ds, 0x5CC9, p->cy);
    w16(ds, 0x5CCB, (p->cy + 1) * 80 + p->cx);
    w16(ds, 0x3F3B, p->view_top);
    w16(ds, 0x65AA, p->mode == PM_TRACE ? 2 : p->mode == PM_SQUAD ? 1 : p->mode == PM_NORMAL ? 0 : p->mode);
    w16(ds, 0x6312, p->time_div);
    w16(ds, 0x6310, p->tick_count);
    w16(ds, 0x688A, p->ticks);
    w16(ds, 0x6314, p->wind);
    w16(ds, 0x6865, p->wind_on);
    w16(ds, 0x6851, p->fight_delay);
    memcpy(&ds[0x5BFF], p->map, NUM_CELLS);
    for (int k = 0; k < AREA_WORDS; k++) w16(ds, 0x3F3D + 2 * k, p->area[k]);
    for (int k = 0; k < NUM_CELLS; k++) w16(ds, 0x4965 + 2 * k, p->port_sup[k]);
    for (int k = 0; k < MAX_PORTS; k++) {
        w16(ds, 0x689C + 2 * k, p->port_cell[k]);
        w16(ds, 0x6932 + 2 * k, p->port_source[k]);
    }
    w16(ds, 0x6499, p->ports_supply);
    w16(ds, 0x649B, p->ports_repair);
    w16(ds, 0x649D, p->crowns);
    memcpy(&ds[0x4E53], p->path, PATH_SIZE + ROUTE_SIZE);
    w16(ds, 0x62F7, p->path_w);
    w16(ds, 0x64D1, p->trace_skip);
    for (int k = 0; k < NUM_RULES; k++) w16(ds, 0x64AD + 2 * k, p->rules[k]);
    for (int k = 0; k < OPP_WORDS; k++) w16(ds, 0x604D + 2 * k, p->opp[k]);
    w16(ds, 0x645B, p->text_rows);
    w16(ds, 0x645D, p->msg_rows);
    w16(ds, 0x6890, p->msg_last);
    w16(ds, 0x630D, p->msg_timer);
    static const int FLEET[6] = { 0x64C3, 0x64BF, 0x64C1, 0x64C9, 0x64C5, 0x64C7 };
    for (int k = 0; k < 6; k++) w16(ds, FLEET[k], p->fleet[k]);
    w16(ds, 0x689A, p->dist);
    for (int k = 0; k < 4; k++) w16(ds, 0x64A5 + 2 * k, p->losses[k]);
    w16(ds, 0x64A1, p->captured[0]);
    w16(ds, 0x64A3, p->captured[1]);
    w16(ds, 0x686B, p->battle_shore);
    w16(ds, 0x6301, p->battle_a);
    w16(ds, 0x6303, p->battle_b);
    w16(ds, 0x6309, p->crown_threat);
    w16(ds, 0x6898, p->merchant_lost);
    w16(ds, 0x65B2, p->water_value[0]);
    w16(ds, 0x65B4, p->water_value[1]);
    w8(ds, 0x65BA, p->no_flags[0]);
    w8(ds, 0x65BB, p->no_flags[1]);
    w8(ds, 0x630F, p->zoomed);
    w16(ds, 0x649F, p->result_code);
    w16(ds, 0x64DD, p->ai_done);
    w16(ds, 0x62FF, p->ai_sq);
    w16(ds, 0x64D7, p->ai_range);
    w16(ds, 0x64DF, p->ai_target);
    w16(ds, 0x6495, p->qn);
    w16(ds, 0x69CB, p->ai_reverse);
    w16(ds, 0x69CD, p->ai_dropped);
    w16(ds, 0x0808, p->rnd);
    w16(ds, 0x080A, p->rnd_ptr);
}

void play_from_ds(Play *p, const uint8_t *ds)
{
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
#define X(a, f) q->f = r16(ds, a + 2 * i);
        SQW(F)
#undef X
        int sp = r16(ds, 0x5D2D + 2 * i);
        q->used = sp != NO_DEST;
        if (q->used) q->speed = sp;
        q->supplies = r8(ds, 0x5E1D + i);
        q->mark = r8(ds, 0x5EBD + i);
        q->seen = r16(ds, 0x574F + 2 * i) != 0;
        q->has_path = r16(ds, 0x583F + 2 * i) != 0;
        q->has_route = r16(ds, 0x588F + 2 * i);
        q->port_speed = r8(ds, 0x6581 + i);
        q->routed = r8(ds, 0x5687 + i);
        PShip *s = &p->ship[i];
        s->sq = r8s(ds, 0x5E45 + i);
        s->sails = r8(ds, 0x5E6D + i);
        s->hull = r8(ds, 0x6025 + i);
        s->type = r8(ds, 0x5E95 + i);
    }
    for (int k = 0; k < 3; k++) p->list[k] = r8s(ds, 0x6456 + k);
    memcpy(p->map, &ds[0x5BFF], NUM_CELLS);
    for (int k = 0; k < AREA_WORDS; k++) p->area[k] = (int16_t)r16(ds, 0x3F3D + 2 * k);
    for (int k = 0; k < NUM_CELLS; k++) p->port_sup[k] = r16(ds, 0x4965 + 2 * k);
    for (int k = 0; k < MAX_PORTS; k++) {
        p->port_cell[k] = r16(ds, 0x689C + 2 * k);
        p->port_source[k] = r16(ds, 0x6932 + 2 * k);
    }
    memcpy(p->path, &ds[0x4E53], PATH_SIZE + ROUTE_SIZE);
    p->path_w = r16(ds, 0x62F7);
    for (int k = 0; k < OPP_WORDS; k++) p->opp[k] = (int16_t)r16(ds, 0x604D + 2 * k);
    p->crown_threat = r16(ds, 0x6309);
    p->merchant_lost = r16(ds, 0x6898);
    p->ai_done = r16(ds, 0x64DD);
    p->ai_sq = r16(ds, 0x62FF);
    p->ai_range = r16(ds, 0x64D7);
    p->ai_target = r16(ds, 0x64DF);
    p->qn = r16(ds, 0x6495);
    p->ai_reverse = r16(ds, 0x69CB);
    p->ai_dropped = r16(ds, 0x69CD);
    p->rnd = (uint16_t)r16(ds, 0x0808);
    p->rnd_ptr = (uint16_t)r16(ds, 0x080A);
}

/* ------------------------------------------------------------------ the saved game (step 6)
 * SS is DS:3F39-6804 as save_game (ovl1:7B29) writes it: the play's state
 * above, with [3F39] as an offset from BUF, SO5's messages where campaign_play
 * loaded them (DS:4AF5), the ship names (609F), the opponent (646F). */

void play_save_ds(const Play *p, uint8_t *ds)
{
    size_t n5 = SDL_min(sizeof p->ga->so5, (size_t)(0x4E53 - 0x4AF5));
    memcpy(&ds[0x4AF5], p->ga->so5, n5);
    play_to_ds(p, ds);
    w16(ds, 0x3F39, p->view_top * 80);
    for (int i = 0; i < MAX_SHIPS; i++) {
        const char *name = p->c ? p->c->ships[i].name : "";
        for (int k = 0; k < SHIP_NAME_LEN; k++) {
            w8(ds, 0x609F + i * SHIP_NAME_LEN + k, (uint8_t)*name);
            if (*name) name++;
        }
    }
    w16(ds, 0x646F, p->c ? p->c->opponent : 0);
    w16(ds, 0x6465, 0);                      /* not a new game */
    w16(ds, 0x6493, 0);                      /* not the demo */
    w16(ds, 0x6497, 1);                      /* the data disk is in the drive */
    w16(ds, 0x65A9, 0);
    w16(ds, 0x65B8, 0);
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

bool play_load_ds(Play *p, Campaign *c, const GameAssets *ga, const uint8_t *ds, uint16_t rnd, uint16_t rnd_ptr)
{
    /* the campaign as it stands, in an S file's layout */
    uint8_t s[0x884];
    memset(s, ' ', 0x348);
    memset(s + 0x348, 0, sizeof s - 0x348);
    memcpy(s + 0x348, &ds[0x5BFF], NUM_CELLS);
    memcpy(s + 0x410, &ds[0x5CDD], 0x1E0);
    memcpy(s + 0x5F0, &ds[0x646F], 2);
    memcpy(s + 0x5F2, &ds[0x64AD], 2 * NUM_RULES);
    memcpy(s + 0x604, &ds[0x609F], MAX_SHIPS * SHIP_NAME_LEN);
    memcpy(s + 0x85C, &ds[0x6025], MAX_SHIPS);
    if (!campaign_parse(c, "SS", s, sizeof s)) return false;
    SDL_strlcpy(c->title, "Saved game", sizeof c->title);

    memset(p, 0, sizeof *p);
    p->c = c;
    p->ga = ga;
    play_from_ds(p, ds);
    p->rnd = rnd;                            /* random() goes on from the menus (DS:0808 is not in SS) */
    p->rnd_ptr = rnd_ptr;
    for (int i = 0; i < MAX_SHIPS; i++) {    /* whatever the file holds, the indices stay in range */
        PSquad *q = &p->sq[i];
        q->x = clampi(q->x, 0, 79);
        q->y = clampi(q->y, 0, 198);
        q->heading &= 15;
        q->speed = clampi(q->speed, 0, 2);
        q->supplies = clampi(q->supplies, 0, 255);
        q->partner = clampi(q->partner, 0, MAX_SHIPS);
        q->path_pos = clampi(q->path_pos, 0, PATH_SIZE + ROUTE_SIZE - 1);
        q->route_pos = clampi(q->route_pos, 0, PATH_SIZE + ROUTE_SIZE - 1);
        q->merchant_dest = clampi(q->merchant_dest, -1, NUM_CELLS - 1);
        q->dest = NO_DEST;                   /* load_saved_game: [6316] = 9999 */
        q->side = q->side ? -1 : 0;
        if (q->ctrl < -2 || q->ctrl > 10000) q->ctrl = -2;
        PShip *sh = &p->ship[i];
        if (sh->sq < -1 || sh->sq >= MAX_SHIPS) sh->sq = -1;
        sh->sails = clampi(sh->sails, 0, 5);
        sh->hull = clampi(sh->hull, 0, 5);
        if (sh->type > 3 && sh->type != 255) sh->type = 2;   /* 0-3, an unused ship 255 */
    }
    for (int k = 0; k < 3; k++) if (p->list[k] < -1 || p->list[k] >= MAX_SHIPS) p->list[k] = -1;
    for (int k = 0; k < NUM_CELLS; k++) if (p->map[k] < 1 || p->map[k] > NUM_TILES) p->map[k] = 1;
    p->sel = clampi(r16(ds, 0x62FB), -1, MAX_SHIPS - 1);
    p->free_slot = clampi(r16(ds, 0x62FD), -1, MAX_SHIPS - 1);
    p->cx = clampi(r16(ds, 0x5CC7), 1, 78);
    p->cy = clampi(r16(ds, 0x5CC9), 0, 195);
    p->view_top = clampi(r16(ds, 0x3F3B), 0, VIEW_TOP_MAX);
    int t = r16(ds, 0x6312);
    p->time_div = (t == 1 || t == 4 || t == 16) ? t : 64;
    p->tick_count = clampi(r16(ds, 0x6310), 0, 64);
    p->wind = r16(ds, 0x6314);               /* 0-3, or -1 without wind */
    if (p->wind < -1 || p->wind > 3) p->wind = 0;
    p->wind_on = assets_scnf_word(ga, 0x6865) != 0;
    p->fight_delay = assets_scnf_word(ga, 0x6851);
    if (p->fight_delay <= 6) p->fight_delay = 500;
    p->ports_supply = r16(ds, 0x6499);
    p->ports_repair = r16(ds, 0x649B);
    p->crowns = r16(ds, 0x649D);
    for (int k = 0; k < NUM_RULES; k++) p->rules[k] = r16(ds, 0x64AD + 2 * k);
    p->path_w = clampi(p->path_w, 0, PATH_SIZE - 1);
    p->text_rows = clampi(r16(ds, 0x645B), 2, 25);   /* campaign_play sets 2 again (play_resume) */
    p->msg_rows = clampi(r16(ds, 0x645D), 0, 2);
    p->msg_timer = r16(ds, 0x630D);
    static const int FLEET[6] = { 0x64C3, 0x64BF, 0x64C1, 0x64C9, 0x64C5, 0x64C7 };
    for (int k = 0; k < 6; k++) p->fleet[k] = r16(ds, FLEET[k]);
    for (int k = 0; k < 4; k++) p->losses[k] = clampi(r16(ds, 0x64A5 + 2 * k), 0, 99);
    p->captured[0] = clampi(r16(ds, 0x64A1), 0, 99);
    p->captured[1] = clampi(r16(ds, 0x64A3), 0, 99);
    p->battle_a = clampi(r16(ds, 0x6301), -1, MAX_SHIPS - 1);
    p->battle_b = clampi(r16(ds, 0x6303), -1, MAX_SHIPS - 1);
    p->crown_threat = clampi(p->crown_threat, -1, NUM_CELLS - 1);
    p->water_value[0] = r16(ds, 0x65B2);
    p->water_value[1] = r16(ds, 0x65B4);
    p->no_flags[0] = r8(ds, 0x65BA) != 0;
    p->no_flags[1] = r8(ds, 0x65BB) != 0;
    p->zoomed = r8(ds, 0x630F) != 0;
    p->result_code = r16(ds, 0x649F);
    p->ai_sq = clampi(p->ai_sq, 0, MAX_SHIPS - 1);
    p->ai_done = clampi(p->ai_done, 0, MAX_SHIPS);
    p->ai_target = clampi(p->ai_target, 0, NUM_CELLS - 1);
    p->qn = clampi(p->qn, 0, 100);
    p->mode = PM_NORMAL;
    int16_t keep[AREA_WORDS];                /* DS:689C is not in SS: found again (WAS.EXE keeps the last game's), */
    memcpy(keep, p->area, sizeof keep);      /* leaving the work arrays as they were saved */
    port_sources(p);
    memcpy(p->area, keep, sizeof keep);
    return true;
}
