/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * play.c: step 3, the strategic game (campaign play, WAR.EXE overlay 6).
 *
 * Each function names the WAR.EXE routine it follows (see
 * WAR_Analysis/WAR.EXE.asm). Encounters stop squads but battles, the enemy's
 * moves and Formation/Detach/Join/Zoom come in later steps.
 */
#include "play.h"
#include "sound.h"

#include <SDL3/SDL.h>

/* Cursor steps for scan codes 47h-51h (DS:04F2, DS:04FD; DS:0508 = dy*80+dx) */
static const int8_t STEP_DX[11] = { -1, 0, 1, 0, -1, 0, 1, 0, -1, 0, 1 };
static const int8_t STEP_DY[11] = { -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1 };

#define CX_MAX        0x4E     /* cursor bounds; 4Dh and C1h while tracing */
#define CY_MAX        0xC3
#define VIEW_MARGIN   0x56     /* cursor may be this far below the view top */
#define RULE_SEEN_ALL 80       /* "enemy seen" rule at or above this: always seen */

/* map tiles (numbers as stored in the T files) */
static bool tile_fort(int t)    { return t == 3; }
static bool tile_village(int t) { return t == 4; }
static bool tile_hill(int t)    { return t == 5 || t == 6; }                      /* rt:11F8 */
static bool tile_bridge(int t)  { return t == 9 || t == 10; }                     /* rt:1228 */
static bool tile_clear(int t)   { return tile_fort(t) || tile_village(t) || tile_hill(t) || tile_bridge(t); }
static bool tile_water(int t)   { return t == 7 || t == 8 || (t >= 13 && t <= 15) || (t >= 19 && t <= 36); }  /* rt:1258 */
static bool tile_mountain(int t){ return (t >= 37 && t <= 54) || t == 65 || t == 66 || t == 71 || t == 72; }  /* rt:1048 */

enum { TER_CLEAR = 0x11, TER_OPEN = 0x44, TER_FOREST = 0x55, TER_WATER = 0x66, TER_MOUNTAIN = 0x99 };

/* ------------------------------------------------------------------ helpers */

static int next_slot(int i) { return i + 1 >= PATH_SIZE ? 0 : i + 1; }      /* rt:1369 */
static int prev_slot(int i) { return i - 1 < 0 ? PATH_SIZE - 1 : i - 1; }

/* random() (rt:20AA): adds successive words of WAR.EXE's own code, 0-127. */
static int rnd(Play *p)
{
    if (++p->rnd_ptr == 0x2328) p->rnd_ptr = 0xBB8;
    unsigned w = p->ga->code[p->rnd_ptr] | (p->ga->code[p->rnd_ptr + 1] << 8);
    p->rnd = (uint16_t)((w + p->rnd) & 0x7F);
    return p->rnd;
}

static int rule(const Play *p, RuleId r) { return (int16_t)p->c->rules[r]; }

/* the opponent's values, by their WAR.EXE address (A4DC-A500) */
#define OPP(p, addr) ((p)->opp[((addr) - 0xA4DC) / 2])

static int sar(int v, int n) { return v >= 0 ? v >> n : -((-v - 1) >> n) - 1; }   /* 8086 SAR */

static const char *DS(const Play *p, unsigned off) { return assets_ds_string(p->ga, off); }
static const char *O5(const Play *p, unsigned off) { return assets_o5_string(p->ga, off); }

/* ------------------------------------------------------------------ text area */

static void tpos(Play *p, int row, int col) { p->trow = row; p->tcol = col; }   /* goto_xy */

static void clear_row(Play *p, int row)    /* clear_text_row */
{
    if (row < 0 || row >= 25) return;
    SDL_memset(p->text[row], ' ', 40);
    p->text[row][40] = 0;
    tpos(p, row, 0);
}

static void print(Play *p, const char *s)  /* print_string / put_char */
{
    for (; *s; s++) {
        if (*s == '\b') { if (p->tcol > 0) p->tcol--; continue; }
        if (p->trow >= 0 && p->trow < 25 && p->tcol < 40) p->text[p->trow][p->tcol] = *s;
        if (++p->tcol >= 40) { p->tcol = 0; p->trow++; }
    }
}

static void print_num(Play *p, int n)
{
    char b[16];
    SDL_snprintf(b, sizeof b, "%d", n);
    print(p, b);
}

static void spaces(Play *p, int n) { while (n-- > 0) print(p, " "); }

/* rt:055B: grow (or shrink) the text area below the map by n rows. */
static void text_rows_add(Play *p, int n)
{
    int old = p->text_rows;
    p->text_rows = SDL_clamp(p->text_rows + n, 2, 25);
    if (p->text_rows < 3) p->msg_rows = 0;
    for (int r = 25 - p->text_rows; r < 25 - old; r++) clear_row(p, r);
}

/* rt:0F80: back to two text rows, row 23 cleared. */
static void text_reset(Play *p)
{
    if (p->text_rows > 2) text_rows_add(p, 2 - p->text_rows);
    clear_row(p, 23);
}

/* ------------------------------------------------------------------ command lines */

static void show_time(Play *p)         /* ovl6:6C0B */
{
    tpos(p, 24, 12);
    print(p, DS(p, p->time_div == 1 ? 0x234 : p->time_div == 4 ? 0x23B : p->time_div == 16 ? 0x240 : 0x247));
    print(p, DS(p, 0x24C));
}

static void show_clear(Play *p)        /* ovl6:6C63 */
{
    if (p->mode != PM_NORMAL) return;
    tpos(p, 24, 21);
    if (p->msg_rows) print(p, DS(p, 0x250));
    else spaces(p, 5);
}

static void cmd_normal(Play *p)        /* ovl6:6BAC: View  Time (slow)  Surrender */
{
    if (p->mode == PM_PROMPT) return;
    text_reset(p);
    clear_row(p, 24);
    print(p, DS(p, 0x228));
    p->mode = PM_NORMAL;
    tpos(p, 24, 6);
    print(p, DS(p, 0x22D));
    show_time(p);
    show_clear(p);
    tpos(p, 24, 29);
    print(p, O5(p, 0x2BF));
}

/* rt:1523: a friendly squad close enough to join (returns index+1, or 0). */
static int join_partner(const Play *p, int i)
{
    const PSquad *a = &p->sq[i];
    if (a->fighting || a->men >= 14) return 0;
    for (int j = 0; j < MAX_SQUADS; j++) {
        const PSquad *b = &p->sq[j];
        if (j == i || !b->used || b->friendly != a->friendly || b->fighting) continue;
        if (SDL_abs(b->x - a->x) < 3 && SDL_abs(b->y - a->y) < 7) return j + 1;
    }
    return 0;
}

static void cmd_squad_line(Play *p)    /* ovl6:6CDD: Move Info Size Formation Detach Join */
{
    const PSquad *q = &p->sq[p->sel];
    clear_row(p, 24);
    bool mine = q->friendly && !q->fighting;
    if (mine) print(p, DS(p, 0x256));
    print(p, O5(p, 0x2C9));
    if (mine) {
        if (!q->small) print(p, O5(p, 0x2D4));                     /* Formation */
        if (q->men > 1 && p->free_slot > -1) print(p, O5(p, 0x2B7));   /* Detach */
        if (join_partner(p, p->sel)) print(p, DS(p, 0x25C));      /* Join */
    }
    if (q->fighting && q->clock > 0) print(p, DS(p, 0x262));       /* Zoom, until the fighting starts */
    p->mode = PM_SQUAD;
}

static void cmd_squad(Play *p)         /* ovl6:6CA3 */
{
    text_reset(p);
    p->free_slot = -1;
    for (int i = MAX_SQUADS - 1; i >= 0; i--)
        if (!p->sq[i].used) { p->free_slot = i; break; }
    cmd_squad_line(p);
}

/* ------------------------------------------------------------------ messages */

static void message(Play *p)           /* ovl6:5CC5: a new message row */
{
    if (p->msg_rows < 2) {
        text_rows_add(p, 1);
        p->msg_rows++;
    } else {                            /* scroll_text_area: row 21 moves down to 22 */
        SDL_memcpy(p->text[22], p->text[21], sizeof p->text[22]);
        clear_row(p, 21);
    }
    tpos(p, 25 - p->text_rows, 1);
}

static int where_code(int x, int y)    /* ovl6:5D00 */
{
    int w = y < 60 ? -1 : y > 140 ? 1 : 0;
    return w + (x < 30 ? -2 : x > 50 ? 2 : 0);
}

static void print_where(Play *p, int x, int y)   /* ovl6:5D42: " IN THE NORTH" */
{
    snd_alert();                                 /* ovl6:5D48 */
    print(p, O5(p, 0x29D));
    bool ns = false;
    if (y < 60)       { print(p, DS(p, 0x1E9)); ns = true; if (p->demo) p->demo_key = PK_KP_MINUS; }   /* the demo */
    else if (y > 140) { print(p, DS(p, 0x1EF)); ns = true; if (p->demo) p->demo_key = PK_KP_PLUS; }    /* scrolls to it */
    if (x < 30)       print(p, DS(p, 0x1F5));
    else if (x > 50)  print(p, DS(p, 0x1FA));
    else if (!ns)     print(p, O5(p, 0x2A6));
    int r = p->trow, c = p->tcol;
    show_clear(p);
    tpos(p, r, c);
}

static void alert_sighted(Play *p, int j)        /* ovl6:59FC */
{
    int w = where_code(p->sq[j].x, p->sq[j].y) + 5;
    if (w == p->alert_where) return;
    p->alert_where = w;
    p->alert_timer = 10;
    message(p);
    print(p, O5(p, 0xEA));
    print_where(p, p->sq[j].x, p->sq[j].y);
}

static void cmd_squad_line(Play *p);

/* ovl6:5A6E, 5AC7, 5B1E, 5B6E: ENCOUNTER, FIGHTING, BATTLE WON/LOST, CAPTURED.
 * Prints a, then b (each "" to skip), then where squad i is. */
static void alert_battle(Play *p, int i, const char *a, const char *b)
{
    p->alert_where = 0;
    message(p);
    print(p, a);
    print(p, b);
    print_where(p, p->sq[i].x, p->sq[i].y);
    if (p->mode == PM_SQUAD) {
        int r = p->trow, c = p->tcol;
        cmd_squad_line(p);
        tpos(p, r, c);
    }
}

static void alert_lost(Play *p, int i, int terrain)   /* ovl6:59A5 */
{
    p->alert_where = 0;
    message(p);
    print(p, O5(p, 0xC4));
    if (terrain == TER_MOUNTAIN) {
        print(p, O5(p, 0xD4));
    } else {
        print(p, O5(p, 0x28B));
        print_where(p, p->sq[i].x, p->sq[i].y);
    }
}

/* ------------------------------------------------------------------ terrain */

static int cell_of(const PSquad *q) { return ((q->y + 5) / 10) * 10 + ((q->x + 1) >> 3); }   /* rt:1330 */

static int tile_at_cell(const Play *p, int cell)
{
    if (cell < 0 || cell >= MAP_W * MAP_H) return 0;
    return p->c->map[cell / MAP_W][cell % MAP_W];
}

/* rt:1D2E: does one of the four pixel pairs in bytes x, x+1 of a map line
 * make the 2bpp nibble code? */
static bool line_has(const Play *p, int x, int line, int code)
{
    if (line < 0 || line >= MAP_PIXEL_H) return false;
    for (int k = 0; k < 4; k++) {
        int px = x * 4 + k * 2;
        if (px < 0 || px + 1 >= SCREEN_W) continue;
        int nib = (p->terrain[line][px] << 2) | p->terrain[line][px + 1];
        if (nib == (code & 0xF)) return true;
    }
    return false;
}

/* ovl6:603E: the ground under a squad's feet, read from the CGA map pixels. */
static int terrain_of(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    int t = tile_at_cell(p, cell_of(q));
    if (tile_clear(t)) return TER_OPEN;
    int line = 2 * q->y + 10;
    for (int pass = 0; pass < 2; pass++, line = 2 * q->y + 8) {
        if (line_has(p, q->x, line, 0x55)) return TER_FOREST;
        if (line_has(p, q->x, line, 0x44)) return TER_OPEN;
        if (line_has(p, q->x, line, 0x66) && tile_water(t)) {
            if (pass) return TER_WATER;
            continue;
        }
        if (tile_mountain(t)) {
            if (line_has(p, q->x, line, 0x99)) return TER_MOUNTAIN;
            if (line_has(p, q->x, line, 0x00)) {
                if (pass) return TER_MOUNTAIN;
                continue;
            }
        }
        return TER_CLEAR;
    }
    return TER_CLEAR;
}

/* ------------------------------------------------------------------ squads */

/* the enemy commander (below) */
static void ai_cell_costs(Play *p);
static void ai_cell_values(Play *p);
static void ai_choose_attack_flag(Play *p);
static void ai_tick(Play *p);
static void ai_start(Play *p);
static void ai_all_out(Play *p);
static int  count_flags(Play *p);
static void enemy_split_in_fort(Play *p, int i);
static void new_squad_from(Play *p, int i, int k);
static void cmd_squad(Play *p);

/* rt:0C44 clear_squad_path: clear the squad's path, its route (enemy), and
 * stop it */
static void clear_path(Play *p, int i)
{
    PSquad *q = &p->sq[i];
    if (q->has_path) {
        int k = q->path_pos;
        do {
            p->path[k] = 0;
            k = next_slot(k);
        } while (p->path[k] != 0 && k != q->path_pos);
        q->marked = false;
        q->has_path = false;
    }
    if (q->on_route) {
        int k = q->route_pos;
        for (;;) {
            if (k >= 0 && k < MAP_W * MAP_H) p->route[k] = 0;
            if (++k > 199 || p->route[k] == 0) break;
        }
        q->on_route = 0;
    }
    if (q->used) {
        q->delay = 0;
        q->moving = 0;
    }
}

static void stop_squad(Play *p, int i)  /* ovl6:47C5 */
{
    p->sq[i].moving = 0;
    p->sq[i].marked = false;
    clear_path(p, i);
}

/* ovl6:47F8: start of the longest free run in the path memory, or -1 if
 * the longest is under 10 steps. */
static int free_run(const Play *p)
{
    int best_len = 0, start = next_slot(p->path_next), best = start, pos = start;
    for (;;) {
        int len = 0;
        while (p->path[pos] == 0 && pos != p->path_next) { len++; pos = next_slot(pos); }
        if (len > best_len) { best_len = len; best = start; }
        while (p->path[pos] != 0 && pos != p->path_next) pos = next_slot(pos);
        start = pos;
        if (pos == p->path_next) break;
    }
    return best_len < 10 ? -1 : best;
}

/* ovl6:50D9: a squad loses a man (attacker == defender for terrain losses).
 * men is left without the spies until the next count_men. */
static void lose_man(Play *p, int att, int def)
{
    PSquad *a = &p->sq[att], *d = &p->sq[def];
    if (!d->knights && !d->barbarians && !d->archers) { d->men = 0; d->spies = 0; return; }
    bool killed = false;
    if (rnd(p) < 64) {
        bool pa = a->archers && d->knights;       /* archers beat knights */
        bool pk = a->knights && d->barbarians;    /* knights beat barbarians */
        bool pb = a->barbarians && d->archers;    /* barbarians beat archers */
        while (!killed && (pa || pk || pb)) {
            int r = rnd(p);
            if (r < 42)      { if (pa) { d->knights--; killed = true; } }
            else if (r < 84) { if (pk) { d->barbarians--; killed = true; } }
            else             { if (pb) { d->archers--; killed = true; } }
        }
    }
    if (rnd(p) < 32) {
        while (!killed) {
            int r = rnd(p);
            if (r < 42)      { if (d->archers) { d->archers--; killed = true; } }
            else if (r < 84) { if (d->knights) { d->knights--; killed = true; } }
            else             { if (d->barbarians) { d->barbarians--; killed = true; } }
        }
    }
    d->men = d->archers + d->knights + d->barbarians;
    if (killed) {
        if (d->friendly) p->lost_mine++;
        else p->lost_theirs++;
    }
}

/* rt:0BD5: men = archers + knights + barbarians + spies */
static void count_men(Play *p)
{
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        q->men = q->archers + q->knights + q->barbarians + q->spies;
        if (q->men < 0) { q->spies = 0; q->men = 0; }
    }
}

/* rt:0B27: recount the men, mark squads of spies only, remove empty squads. */
static void squads_update(Play *p)
{
    count_men(p);
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        q->small = q->spies && !q->archers && !q->knights && !q->barbarians;
        if (q->men == 0 && q->used) {
            clear_path(p, i);
            q->used = false;
            q->moving = 0;
            if (p->sel == i && p->mode == PM_SQUAD) { p->over_squad = false; cmd_normal(p); }
        } else if (q->men > 0 && !q->used) {   /* a new squad (Detach, or an enemy squad splitting) */
            q->used = true;
            q->moving = 0;
        }
    }
}

/* ovl6:380F: the game is over; every squad is shown and Enter ends it. */
static void game_over(Play *p, bool won)
{
    snd_end_of_turn();                      /* ovl6:3812 */
    p->reveal = true;
    p->mode = PM_PROMPT;
    p->ended = true;
    p->enter_icon = true;
    p->outcome = won ? PLAY_WON : PLAY_LOST;
    text_reset(p);
    clear_row(p, 24);
    tpos(p, 24, won ? 3 : 2);
    print(p, O5(p, won ? 0x282 : 0x278));   /* You Won / You Lost */
    print(p, O5(p, 0xB1));                  /* ...  [Enter]  to continue. */
}

/* ovl6:3589: has one side no squads left, or will the opponent surrender?
 * Returns true if play has stopped. */
static bool check_victory(Play *p, int a, int b)
{
    int mine = 0, theirs = 0, my_men = 0, their_men = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if ((i == a || i == b) && q->has_path) q->moving = q->speed;   /* carry on after a battle */
        if (!q->used) continue;
        if (q->friendly) { mine++; my_men += q->men; }
        else             { theirs++; their_men += q->men; }
    }
    if (theirs / 2 < OPP(p, 0xA4E8)) OPP(p, 0xA4E8)--;
    if (mine < 3 || theirs / 4 > mine || theirs < 3) ai_all_out(p);   /* the end is near: all out */
    if (mine == 0)   { game_over(p, false); p->result_code = 12; return true; }
    if (theirs == 0) { game_over(p, true); p->result_code = 2; return true; }
    if (their_men > 0 && my_men / their_men >= OPP(p, 0xA4EA) && my_men > 20) {
        OPP(p, 0xA4EA) = 500;               /* only offered once */
        snd_alert();                        /* ovl6:373D */
        p->mode = PM_PROMPT;
        p->prompt_offer = true;
        for (int r = 3; r <= 6; r++) clear_row(p, r);
        tpos(p, 4, 0);
        print(p, O5(p, 0x6E));              /* Your opponent offers to surrender. */
        tpos(p, 5, 0);
        print(p, O5(p, 0x94));              /* Do you accept? (Y or N) */
        return true;
    }
    return false;
}

/* ovl6:57EA: ticks per step for every squad, from its speed and the ground;
 * hazards in deep water and high mountains. Returns true if the game ended. */
static bool update_delays(Play *p, bool hazards)
{
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used) continue;
        /* enemy squads use their commander's values for forest and mountains */
        int forest = q->friendly ? rule(p, RULE_FOREST) : OPP(p, 0xA4FC);
        int mountain = q->friendly ? rule(p, RULE_MOUNTAIN) : OPP(p, 0xA4FA);
        int water = rule(p, RULE_WATER);
        q->delay = q->moving;
        int t = terrain_of(p, i);
        if (t == TER_FOREST) q->delay *= forest;
        else if (t == TER_MOUNTAIN) q->delay *= mountain;
        else if (t == TER_WATER) q->delay *= water;
        if (q->knights) q->delay += q->moving;      /* horses are slow in rough ground */
        if (!hazards || !q->moving) continue;

        bool lose = false;
        if ((t == TER_MOUNTAIN && mountain >= 16) || (t == TER_WATER && water >= 20)) lose = rnd(p) < 2;
        if (!lose && t == TER_WATER && water >= 40) lose = rnd(p) < 4;
        if (!lose || q->fighting) continue;
        lose_man(p, i, i);
        if (q->men > 0) continue;
        if (q->friendly) alert_lost(p, i, t);
        squads_update(p);
        if (check_victory(p, i, i)) return true;
    }
    return false;
}

/* ovl6:5DF2: squads take their next steps. */
static void move_squads(Play *p)
{
    if (update_delays(p, true)) return;
    bool friendly_moved = false;
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used || !q->has_path || !q->moving || q->fighting) continue;
        if (--q->countdown > 0) continue;
        if (q->friendly) friendly_moved = true;
        q->countdown = q->delay;
        int step = p->path[q->path_pos];
        p->path[q->path_pos] = 0;
        if (step == PATH_END) {
            q->marked = false;
            q->moving = 0;
            q->has_path = false;
            continue;
        }
        if (step > 78)       { q->y++; step -= 80; }
        else if (step < -78) { q->y--; step += 80; }
        q->x += step;
        q->path_pos = next_slot(q->path_pos);
        if (p->mode != PM_NORMAL && p->mode != PM_TRACE && p->sel == i &&
            (rule(p, RULE_ENEMY_SEEN) >= RULE_SEEN_ALL || q->friendly || q->seen)) {
            p->cx = q->x + 1;    /* the cursor follows the squad */
            p->cy = q->y;
        }
    }
    if (friendly_moved && p->mode == PM_SQUAD) cmd_squad_line(p);
}

/* rt:08EF: -30..30, how much better placed squad i is than j: condition,
 * hills, and water or spies-only squads at a great disadvantage */
static int edge(const Play *p, int i, int j)
{
    const PSquad *a = &p->sq[i], *b = &p->sq[j];
    int ti = tile_at_cell(p, cell_of(a)), tj = tile_at_cell(p, cell_of(b));
    int d = a->condition - b->condition;
    if (tile_hill(ti)) d += 15;
    if (tile_hill(tj)) d -= 15;
    if (tile_water(ti) || a->small) d = -30;
    if (tile_water(tj) || b->small) d = 30;
    return SDL_clamp(d, -30, 30);
}

/* rt:0859: strength of squad i against j (over 50 or under -50: a walkover) */
static int strength(const Play *p, int i, int j)
{
    const PSquad *a = &p->sq[i], *b = &p->sq[j];
    if (a->small && b->small) return 0;
    if (a->small) return -67;
    if (b->small) return 67;
    return sar(edge(p, i, j), 1) + (a->men - b->men) * 4;
}

static void check_over(Play *p, int key);

/* ovl6:4C5F: squad k is captured without a fight */
static void capture(Play *p, int k, int i, int j)
{
    PSquad *q = &p->sq[k];
    if (q->friendly) { p->captured_mine += q->men; alert_battle(p, k, O5(p, 0x13C), O5(p, 0x293)); }   /* YOUR SQUAD CAPTURED */
    else             { p->captured_theirs += q->men; alert_battle(p, k, DS(p, 0x1E3), O5(p, 0x293)); } /* ENEMY CAPTURED */
    q->archers = q->knights = q->barbarians = q->spies = 0;
    p->sq[i].fighting = p->sq[j].fighting = false;
    squads_update(p);
    if (check_victory(p, i, j)) return;
    check_over(p, 0);
}

/* ovl6:4E26: every iteration, encounters count down to a fight, and squads
 * fighting on the map (blinking) kill each other's men until one is gone. */
static void battles(Play *p)
{
    p->blink = !p->blink;
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used || !q->fighting || !q->clock) continue;
        PSquad *e = &p->sq[q->foe];
        if (q->clock > 0 && q->clock < p->fight_delay) {
            q->clock++;
            if (p->time_div == 1 && q->clock < p->fight_delay - 1) q->clock += 2;
            continue;
        }
        if (q->clock == p->fight_delay) {      /* too late to Zoom: they fight it out */
            q->clock = FIGHT_ON;
            q->kill = edge(p, i, q->foe) * 2 + 60;
            if (tile_fort(tile_at_cell(p, cell_of(q))) && q->archers) q->kill *= 2;
            if (q->friendly) alert_battle(p, i, O5(p, 0x114), "");   /* A L E R T - FIGHTING */
            continue;
        }
        if (rnd(p) >= q->kill || rnd(p) >= 25) continue;
        lose_man(p, i, q->foe);
        if (e->men > 0) continue;
        int f = q->foe;                         /* the other squad is beaten */
        e->spies = 0;
        if (!e->friendly && rnd(p) > OPP(p, 0xA4E2) && cell_of(e) == p->target_cell)
            p->target_cell = -1;                /* ovl6:4FFC: the enemy gives up that target */
        q->condition = SDL_max(q->condition >> 1, 1);
        alert_battle(p, f, O5(p, 0x12A), DS(p, q->friendly ? 0x1DA : 0x1DE));   /* NOTICE - BATTLE WON/LOST */
        q->fighting = e->fighting = false;
        q->clock = e->clock = 0;
        squads_update(p);
        if (check_victory(p, i, f)) return;
        check_over(p, 0);
    }
}

/* ovl6:49B6: sighting, and squads meeting. */
static void encounters(Play *p)
{
    bool seen[MAX_SQUADS] = { false };
    int seen_rule = rule(p, RULE_ENEMY_SEEN);
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *a = &p->sq[i];
        if (!a->used || !a->friendly || a->fighting) continue;
        int ci = cell_of(a), ti = tile_at_cell(p, ci);
        for (int j = 0; j < MAX_SQUADS; j++) {
            PSquad *b = &p->sq[j];
            if (!b->used || b->friendly || b->fighting) continue;
            int cj = cell_of(b), tj = tile_at_cell(p, cj);
            /* in a fort, or a hill (unless the other is in a fort), a squad
             * counts as being at the middle of the tile */
            int xi = a->x, yi = a->y, xj = b->x, yj = b->y;
            if (tile_fort(ti) || (tile_hill(ti) && !tile_fort(tj))) { xi = (ci % 10) * 8 + 4; yi = (ci / 10) * 10; }
            if (tile_fort(tj) || (tile_hill(tj) && !tile_fort(ti))) { xj = (cj % 10) * 8 + 4; yj = (cj / 10) * 10; }
            int dx = SDL_abs(xi - xj), dy = SDL_abs(yi - yj);
            if (tile_fort(ti) || tile_fort(tj) || tile_hill(ti) || tile_hill(tj)) {
                dx = SDL_max(dx - 3, 0);
                dy = SDL_max(dy - 3, 0);
            }
            int d = 2 * dx + dy;
            if (a->spies) d >>= 1;
            if (dx < 3 && dy < 5) {
                if (a->small && b->small) continue;      /* spies slip past each other */
                /* a fort keeps out squads without archers */
                if (tile_fort(ti) && !tile_fort(tj) && !a->archers) continue;
                if (!tile_fort(ti) && tile_fort(tj) && !b->archers) continue;
                int r = strength(p, i, j);
                if ((r < -50 && !tile_fort(ti)) || (r > 50 && !tile_fort(tj))) {
                    capture(p, r < -50 ? i : j, i, j);   /* one side gives up without a fight */
                    if (p->mode == PM_PROMPT) return;
                    break;
                }
                seen[i] = seen[j] = true;               /* ovl6:4D01: the squads stop and face off */
                a->fighting = b->fighting = true;
                a->foe = j;
                b->foe = i;
                alert_battle(p, i, O5(p, 0x100), "");   /* WARNING - ENCOUNTER */
                if (p->demo) { p->demo = 2; p->sel = i; }   /* ovl6:5AAD: the demo zooms to it */
                a->clock = b->clock = 1;
                a->moving = b->moving = 0;
                break;
            }
            if ((d < seen_rule && seen_rule < RULE_SEEN_ALL) || (seen_rule >= RULE_SEEN_ALL && d < 20)) {
                seen[i] = seen[j] = true;
                if (!b->seen) alert_sighted(p, j);
            }
        }
    }
    for (int k = 0; k < MAX_SQUADS; k++) p->sq[k].seen = seen[k] || p->sq[k].fighting;
}


/* ------------------------------------------------------------------ supply, food, training, flags */

static int cell_x(int c) { return (c % 10) * 8 + 4; }      /* ovl6:7588: middle of a cell */
static int cell_y(int c) { return (c / 10) * 10; }         /* ovl6:75A4: top of a cell */

/* ovl6:75BB: steps from one place to another, moving diagonally */
static int step_distance(int x, int y, int tx, int ty) { return SDL_max(SDL_abs(tx - x), SDL_abs(ty - y)); }

/* ovl6:873A: how strongly each side holds each map cell. Each squad counts
 * 4 per man (minus for yours): nothing in water, a quarter in the mountains,
 * double in a fort or on a hill, half if worn out. This spreads twice to the
 * neighbouring cells, an eighth at a time (a quarter of that into mountains);
 * water cells hold nothing. */
static void supply_map(Play *p)
{
    static const int8_t NB[9] = { 0, 1, 11, 10, 9, -1, -11, -10, -9 };   /* O5 words 1-8 */
    int str[MAP_W * MAP_H] = { 0 };
    count_men(p);
    SDL_memset(p->supply, 0, sizeof p->supply);
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used) continue;
        int v = q->men * 4;
        if (q->friendly) v = -v;
        int c = cell_of(q), t = tile_at_cell(p, c);
        if (c < 0 || c >= MAP_W * MAP_H) continue;
        if (tile_water(t)) v = 0;
        else if (tile_mountain(t)) v = sar(v, 2);
        else if (tile_fort(t) || tile_hill(t)) v *= 2;
        if (q->condition < 10) v = sar(v, 1);
        str[c] += v;
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int c = 0; c < MAP_W * MAP_H; c++) {
            int t = tile_at_cell(p, c);
            if (tile_water(t)) continue;
            int col = c % 10;
            bool mtn = tile_mountain(t);
            for (int k = 1; k <= 8; k++) {
                if (c < 10 && k > 5) continue;                          /* top row */
                if (c > 189 && k > 1 && k < 5) continue;               /* bottom row */
                if (col == 0 && k > 3 && k < 7) continue;              /* left column */
                if (col == 9 && (k == 8 || k == 1 || k == 2)) continue; /* right column */
                int v = sar(str[c + NB[k]], 3);
                if (mtn) v = sar(v, 2);
                p->supply[c] = (int16_t)(p->supply[c] + v);
            }
            p->supply[c] = (int16_t)(p->supply[c] + str[c]);
        }
        if (pass == 0) {
            for (int c = 0; c < MAP_W * MAP_H; c++) str[c] = p->supply[c];
            SDL_memset(p->supply, 0, sizeof p->supply);
        }
    }
}

/* ovl6:6A21: steps to the nearest village or fort that feeds this squad
 * (one its side holds, if the rules let it supply food), divided by the
 * supply line range. */
static int supply_distance(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    int best = 999;
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        int t = tile_at_cell(p, c);
        if (!((tile_village(t) && rule(p, RULE_VILLAGES_SUPPLY_FOOD)) || (tile_fort(t) && rule(p, RULE_FORTS_SUPPLY_FOOD))))
            continue;
        if (q->friendly ? p->supply[c] >= -1 : p->supply[c] <= 1) continue;
        best = SDL_min(best, step_distance(q->x, q->y, cell_x(c), cell_y(c)));
    }
    int range = rule(p, RULE_SUPPLY_LINE_RANGE);
    return range > 0 ? best / range : best;
}

/* ------------------------------------------------------------------ the enemy commander
 *
 * Overlay 6 gives the enemy's orders one squad at a time: each tick the next
 * enemy squad standing still (ai_tick) either stays where it is (a fort, a
 * village, a bridge, a hill or a flag worth holding), or is sent somewhere:
 * to food, to guard its own flag, or to the target the commander has chosen
 * (one of your flags, or a cell worth taking). A route of map cells is
 * planned (ai_route) and the squad marches it a cell at a time, half of it
 * before planning again, turning aside for a flag or for one of your squads
 * it is strong enough to attack. The commander's values from O2 (DS:A4DC)
 * set how bold, careful, hungry or random each general is. */

#define OPPV(p, addr) ((int)(p)->opp[((addr) - 0xA4DC) / 2])

static const int8_t CELL_STEP[9] = { 0, 1, 11, 10, 9, -1, -11, -10, -9 };   /* O5 words 0-8: E, SE, S, SW, W, NW, N, NE */
static const int8_t TURN[8] = { 0, 1, -1, 2, -2, 3, -3, 4 };                /* O5+12h: order of directions to try */

static bool tile_forest(int t) { return (t >= 55 && t <= 57) || (t >= 61 && t <= 63) || (t >= 67 && t <= 69); }   /* rt:10A8 */

static int flag_cell(const Play *p, int f)   /* rt:1383 */
{
    if (f < 0 || f >= MAX_FLAGS) return -1;
    return ((p->flag[f].y + 5) / 10) * 10 + ((p->flag[f].x + 1) >> 3);
}

static int tile_of_cell(const Play *p, int c) { return (c >= 0 && c < MAP_W * MAP_H) ? tile_at_cell(p, c) : 0; }

/* ovl6:3413: what each cell costs a marching enemy, and what forts, villages
 * and bridges are worth (less where there are many) */
static void ai_cell_costs(Play *p)
{
    int water = rule(p, RULE_WATER), mountain = OPPV(p, 0xA4FA);
    if (water > 10) water <<= 1;
    if (mountain > 14) mountain <<= 1;
    p->value_flag = 130;
    p->value_fort = (rule(p, RULE_FORTS_SUPPLY_FOOD) ? 80 : 30) + rule(p, RULE_FORTS_TRAIN_MEN);
    p->value_village = rule(p, RULE_VILLAGES_SUPPLY_FOOD) ? (rule(p, RULE_FORTS_SUPPLY_FOOD) ? 50 : 90) : 5;
    p->value_bridge = 25;
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        int t = tile_at_cell(p, c);
        if (tile_water(t))         p->cell_cost[c] = (int16_t)water;
        else if (tile_mountain(t)) p->cell_cost[c] = (int16_t)mountain;
        else if (tile_forest(t))   p->cell_cost[c] = (int16_t)OPPV(p, 0xA4FC);
        else if (tile_fort(t))     { p->cell_cost[c] = 3; p->value_fort -= 20; }
        else                       p->cell_cost[c] = 1;
        if (tile_village(t)) p->value_village -= 10;
        if (tile_bridge(t)) p->value_bridge -= 5;
    }
    p->value_fort = SDL_max(p->value_fort, 10);
    p->value_village = SDL_max(p->value_village, 5);
    p->value_bridge = SDL_max(p->value_bridge, 5);
}

/* ovl6:8B7E: the worth of each cell; your flags are worth most */
static void ai_cell_values(Play *p)
{
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        int t = tile_at_cell(p, c);
        p->cell_value[c] = (int16_t)(tile_fort(t) ? p->value_fort : tile_village(t) ? p->value_village :
                                     tile_bridge(t) ? p->value_bridge : tile_hill(t) ? 2 : 0);
    }
    for (int f = 0; f < MAX_FLAGS; f++) {
        if (!p->flag[f].present || !p->flag[f].white) continue;
        int c = flag_cell(p, f);
        if (c >= 0 && c < MAP_W * MAP_H) p->cell_value[c] = (int16_t)(p->cell_value[c] + p->value_flag);
    }
}

/* ovl6:760E: steps between two cells, moving diagonally */
static int cell_distance(int a, int b)
{
    return SDL_max(SDL_abs(a % 10 - b % 10), SDL_abs(a / 10 - b / 10));
}

/* ovl6:7E2F: which of your flags to go for: one you hold weakly, near the last one */
static void ai_choose_attack_flag(Play *p)
{
    int ref = p->attack_flag != -1 ? flag_cell(p, p->attack_flag) : rnd(p) + 36;
    p->attack_flag = -1;
    int best = -860;
    for (int f = 0; f < MAX_FLAGS; f++) {
        if (!p->flag[f].present || !p->flag[f].white) continue;
        int c = flag_cell(p, f);
        int v = (c >= 0 && c < MAP_W * MAP_H ? p->supply[c] : 0) - OPPV(p, 0xA4E4);
        if (v > 0) v = 0;
        v -= cell_distance(c, ref) * 8;
        if (rnd(p) < OPPV(p, 0xA4F0)) v -= rnd(p);
        if (v > best) { p->attack_flag = f; best = v; }
    }
}

/* ovl6:7DDE: squads of one side standing in a cell */
static int ai_count_stopped(const Play *p, int cell, bool friendly)
{
    int n = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (q->used && q->moving == 0 && q->friendly == friendly && cell_of(q) == cell) n++;
    }
    return n;
}

/* ovl6:7D6E: enemy squads in a cell or on their way to it */
static int ai_count_heading(const Play *p, int cell)
{
    int n = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (q->used && !q->friendly && ((q->planned && q->dest == cell) || cell_of(q) == cell)) n++;
    }
    return n;
}

/* ovl6:80C1: is the straight line of cells between two cells blocked by water
 * (or by mountains, for a general whose men are slow there)? */
static bool ai_blocked(const Play *p, int from, int to)
{
    for (int n = 0; from != to && n < 40; n++) {
        int dr = to / 10 - from / 10, dc = to % 10 - from % 10;
        if (dr < 0) from -= 10;
        if (dr > 0) from += 10;
        if (dc < 0) from--;
        if (dc > 0) from++;
        int t = tile_of_cell(p, from);
        if (tile_water(t) || (tile_mountain(t) && OPPV(p, 0xA4FA) > 7)) return true;
    }
    return false;
}

/* ovl6:7F0D: the nearest squad of one side, by cells; mode 2 only those the
 * enemy squad is strong enough to attack and can reach. Sets ai_dist. */
static int ai_nearest_squad(Play *p, bool friendly, int cell, int mode)
{
    int best = 999;
    p->ai_dist = 999;
    for (int j = 0; j < MAX_SQUADS; j++) {
        const PSquad *q = &p->sq[j];
        if (!q->used || q->friendly != friendly || q->fighting) continue;
        int c = cell_of(q);
        if (mode == 1 && (tile_village(tile_of_cell(p, c)) || tile_fort(tile_of_cell(p, c)) || c == p->home_cell)) continue;
        if (mode == 2 && (strength(p, p->ai_sq, j) < OPPV(p, 0xA4F4) || ai_blocked(p, cell, c))) continue;
        int d = cell_distance(cell, c);
        if (d < p->ai_dist) { p->ai_dist = d; best = j; }
    }
    p->ai_dist++;
    return best;
}

/* ovl6:7FFC: the nearest flag of one side within reach. Sets ai_dist. */
static int ai_nearest_flag(Play *p, bool white, int reach)
{
    const PSquad *q = &p->sq[p->ai_sq];
    int best = -1, here = cell_of(q);
    for (int f = 0; f < MAX_FLAGS; f++) {
        const PFlag *fl = &p->flag[f];
        if (!fl->present || fl->white != white) continue;
        if (ai_blocked(p, here, flag_cell(p, f))) continue;
        int d = step_distance(fl->x, fl->y, q->x, q->y);
        if (d >= reach || d == 0) continue;
        best = f;
        reach = d;
    }
    p->ai_dist = reach;
    return best;
}

/* ovl6:6B03: the cell to go to for food: a village or fort that feeds,
 * near and held by the enemy (999 if none) */
static int ai_food_cell(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    int best_d = 999, best = 999;
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        int t = tile_at_cell(p, c);
        if (!((tile_village(t) && rule(p, RULE_VILLAGES_SUPPLY_FOOD)) || (tile_fort(t) && rule(p, RULE_FORTS_SUPPLY_FOOD)))) continue;
        int d = step_distance(q->x, q->y, cell_x(c), cell_y(c)) - sar(p->supply[c], 1);
        if (d < best_d) { best_d = d; best = c; }
    }
    return best;
}

/* rt:13BC: steps toward (tx, ty) into the path memory after slot, at most
 * max of them; across only four steps in five. Returns the last slot. */
static int ai_path_steps(Play *p, int tx, int ty, int slot, int max)
{
    const PSquad *q = &p->sq[p->ai_sq];
    int x = q->x + 1, y = q->y, n = 0;
    for (int guard = 0; guard < 4 * PATH_SIZE; guard++) {
        if ((x == tx && y == ty) || max == 0) break;
        int d = 0;
        if (++n >= 5) n = 0;
        else {
            if (x < tx) { d++; x++; }
            if (x > tx) { d--; x--; }
        }
        if (y < ty) { d += 80; y++; }
        if (y > ty) { d -= 80; y--; }
        if (d == 0) continue;
        slot = next_slot(slot);
        if (p->path[slot] != 0) { slot = prev_slot(slot); break; }
        p->path[slot] = (int8_t)d;
        max--;
    }
    return slot;
}

/* ovl6:8176: a path of at most max steps toward (x, y); the squad sets off */
static bool ai_make_path(Play *p, int x, int y, int max)
{
    int i = p->ai_sq;
    PSquad *q = &p->sq[i];
    int slot = free_run(p);
    if (slot == -1) return false;
    q->path_pos = slot;
    int end = ai_path_steps(p, x, y, slot, max);
    if (end == q->path_pos) return true;
    q->path_pos = next_slot(q->path_pos);
    int k = next_slot(end);
    if (p->path[k] != 0) k = prev_slot(k);
    p->path[k] = PATH_END;
    q->countdown = 1;
    q->has_path = true;
    q->moving = q->speed;
    update_delays(p, false);
    return true;
}

/* rt:14A0: start of the longest free run in the route memory */
static int route_free_run(const Play *p)
{
    int start = 0, pos = 0, best = 0, best_len = 0;
    for (;;) {
        int len = 0;
        while (pos < MAP_W * MAP_H && p->route[pos] == 0) { len++; pos++; }
        if (len > best_len) { best_len = len; best = start; }
        while (pos < MAP_W * MAP_H && p->route[pos] != 0) { pos++; if (pos >= 199) break; }
        start = pos;
        if (pos >= 199) break;
    }
    return best;
}

/* ovl6:825E: plan a route of cells from the squad to ai_dest, cheapest
 * first (water, mountains and forest by the general's values, and cells you
 * hold strongly cost more). Half of it (at most 21 cells) goes into the
 * route memory. */
#define OPEN_MAX 400
static bool ai_route(Play *p)
{
    PSquad *q = &p->sq[p->ai_sq];
    int start = cell_of(q), dest = p->ai_dest;
    const int N = MAP_W * MAP_H;
    if (start < 0 || start >= N || dest < 0 || dest >= N) return false;
    int16_t cost[MAP_W * MAP_H], dir[MAP_W * MAP_H], dist[MAP_W * MAP_H];
    int16_t list[OPEN_MAX + 1];
    SDL_memcpy(cost, p->cell_cost, sizeof cost);   /* 8D1C */
    SDL_memset(dir, 0, sizeof dir);                /* 886C */
    SDL_memset(dist, 0, sizeof dist);              /* 89FC */
    for (int c = 0; c < N; c++) {
        int v = OPPV(p, 0xA500) - p->supply[c];
        if (v > 0) cost[c] = (int16_t)(cost[c] + (v >> 1) + 3);
    }
    list[0] = (int16_t)start;                      /* 8EAC, count A6DA */
    int n = 1;
    dir[start] = 9;
    dist[start] = 0;
    bool found = false;
    int dcol = dest % 10;
    while (!found) {
        if (--n < 0) break;
        int pick = 0, low = 999;                   /* the nearest open cell (not looking at the last) */
        for (int k = 0; k < n; k++)
            if (dist[list[k]] < low) { low = dist[list[k]]; pick = k; }
        int cur = list[pick];
        for (int k = pick; k < n; k++) list[k] = list[k + 1];
        int col = cur % 10, d0;
        if (dest < cur) d0 = dcol == col ? 7 : dcol < col ? (cur - 10 >= dest ? 6 : 5) : 8;
        else            d0 = dcol == col ? 3 : dcol > col ? (cur + 10 > dest ? 1 : 2) : 4;
        int d = d0;
        for (int t = 1; t <= 8; t++) {             /* toward the goal first, then turning further away */
            bool edge_skip = (col == 0 && d >= 4 && d <= 6) || (col == 9 && (d == 8 || d == 1 || d == 2));
            int nb = cur + CELL_STEP[d];
            if (!edge_skip && nb >= 0 && nb < N) {
                int nd = dist[cur] + cost[nb];
                if (dir[nb] == 0 || (dir[nb] < 9 && dist[nb] > nd)) {
                    dir[nb] = (int16_t)d;
                    if (n < OPEN_MAX) list[n++] = (int16_t)nb;
                    dist[nb] = (int16_t)nd;
                }
                if (nb == dest) { found = true; break; }
            }
            if (t < 8) {
                d = TURN[t] + d0;
                if (d > 8) d -= 8;
                if (d < 1) d += 8;
            }
        }
    }
    if (!found) return false;
    n = 0;                                         /* back from the goal */
    for (int c = dest;;) {
        list[0] = (int16_t)c;
        if (c == start) break;
        c -= CELL_STEP[dir[c] >= 1 && dir[c] <= 8 ? dir[c] : 0];
        if (++n > 99) return false;
        for (int k = n; k > 0; k--) list[k] = list[k - 1];
    }
    int at = route_free_run(p);
    q->route_pos = at;
    int prev = list[0], steps = 0;
    for (int k = 1; k <= n; k++) {
        int delta = list[k] - prev;
        prev = list[k];
        int code = 8;
        while (code > 0 && CELL_STEP[code] != delta) code--;
        if (at >= 198) { at = 198; break; }
        if (p->route[at] != 0) { at = SDL_max(at - 1, 0); break; }
        if (steps > 20) break;
        p->route[at++] = (int8_t)code;
        steps++;
    }
    p->route[at] = PATH_END;
    q->on_route = 1;
    if (steps > 1) {                               /* march half of it, then plan again */
        int e = q->route_pos + (steps >> 1);
        p->route[e++] = PATH_END;
        while (e <= at) p->route[e++] = 0;
    }
    return true;
}

/* ovl6:570B: move men into a nearby squad of the same side with room */
static void ai_join(Play *p)
{
    int i = p->ai_sq;
    PSquad *q = &p->sq[i];
    int j = join_partner(p, i) - 1;
    if (j < 0) return;
    PSquad *t = &p->sq[j];
    for (int guard = 0; guard < 64 && t->men < 14 && q->men != 0; guard++) {
        if (q->barbarians)   { q->barbarians--; t->barbarians++; }
        else if (q->knights) { q->knights--; t->knights++; }
        else if (q->archers) { q->archers--; t->archers++; }
        else if (q->spies)   { q->spies--; t->spies++; }
        count_men(p);
    }
    if (q->men == 0) squads_update(p);
}

/* rt:161B: a new squad k, standing with squad i */
static void new_squad_from(Play *p, int i, int k)
{
    PSquad *s = &p->sq[i], *n = &p->sq[k];
    n->spies = n->archers = n->barbarians = n->knights = 0;
    n->on_route = 0;
    n->has_path = false;
    n->planned = false;
    n->speed = 2;
    n->x = s->x;
    n->y = s->y;
    if (s->friendly) n->x++;
    if (n->x > 70) n->x -= 2;
    n->condition = s->condition;
    n->food = s->food;
    n->formation = s->formation;
    n->friendly = s->friendly;
    n->fighting = false;
    n->clock = 0;
}

/* ovl6:5613: a full enemy squad standing in a fort splits off a man into the
 * second free squad slot, unless the fort already holds 3 squads */
static void enemy_split_in_fort(Play *p, int i)
{
    PSquad *q = &p->sq[i];
    if (ai_count_stopped(p, cell_of(q), false) > 2) return;
    int k;
    bool seen = false;
    for (k = MAX_SQUADS - 1; k > -1; k--) {
        if (p->sq[k].used || k == p->free_slot) continue;
        if (seen) break;
        seen = true;
    }
    if (k == -1) return;
    new_squad_from(p, i, k);
    PSquad *n = &p->sq[k];
    if (q->barbarians)   { q->barbarians--; n->barbarians = 1; }
    else if (q->knights) { q->knights--; n->knights = 1; }
    else if (q->archers) { q->archers--; n->archers = 1; }
    else                 { q->spies--; n->spies = 1; }
    squads_update(p);
}

static void ai_plan(Play *p);

/* ovl6:7778: a squad standing still: hold its ground, or go somewhere */
static void ai_decide(Play *p)
{
    int i = p->ai_sq;
    PSquad *q = &p->sq[i];
    int here = cell_of(q);
    if (OPPV(p, 0xA4E2) < 150 && ((here >= 0 && here < MAP_W * MAP_H && p->cell_value[here] != 0) || here == p->home_cell)) {
        int ours = ai_count_stopped(p, here, false);
        bool plan = false;
        if (tile_fort(tile_of_cell(p, here))) {        /* keep a fort */
            ai_nearest_squad(p, true, here, 0);
            if (q->men < 14 || ours == 1) return;
            if (p->ai_dist < 4 && q->food > 10) return;
        }
        if (q->food < OPPV(p, 0xA4EC) && supply_distance(p, i) > 9) plan = true;   /* hungry and far from food */
        if (!plan) {
            bool check_hold = true;
            if (rnd(p) <= OPPV(p, 0xA4E6) && rnd(p) >= 100) check_hold = false;
            if (check_hold && !p->ai_hold) return;
            if (here == p->home_cell && OPPV(p, 0xA4E8) >= ours) return;
            if (q->condition < OPPV(p, 0xA4EE)) return;   /* rest first */
        }
    }
    if (here != p->target_cell) ai_plan(p);
}

/* ovl6:7886: where to send a squad: food, its own flag, or the target */
static void ai_plan(Play *p)
{
    int i = p->ai_sq;
    PSquad *q = &p->sq[i];
    int food = ai_food_cell(p, i), here = cell_of(q);
    if (q->food < OPPV(p, 0xA4EC) && food < 999) {
        p->ai_dest = q->dest = food;
    } else if (ai_count_heading(p, p->home_cell) < OPPV(p, 0xA4E8)) {   /* guard the flag, fast */
        q->speed = 1;
        p->ai_dest = q->dest = p->home_cell;
    } else {
        if (ai_count_stopped(p, p->target_cell, false) >= OPPV(p, 0xA4DC) && rnd(p) < 60) {
            p->target_cell = -1;                       /* enough squads there: a new target */
            if (here >= 0 && here < MAP_W * MAP_H && p->cell_value[here]) p->ai_hold = true;
        }
        if (p->target_cell == -1) {
            if (p->attack_flag == -1) ai_choose_attack_flag(p);
            int flag_c = flag_cell(p, p->attack_flag);
            if (rnd(p) < OPPV(p, 0xA4E2)) {
                p->target_cell = flag_c;               /* straight for your flag */
            } else {
                int best = -860;
                for (int c = 0; c < MAP_W * MAP_H; c++) {
                    if (p->cell_value[c] == 0 || c == here) continue;
                    int s = p->supply[c], t = tile_at_cell(p, c);
                    int v = (s < 0 && OPPV(p, 0xA500) < s && !tile_fort(t)) ? 100 : 0;   /* weakly held by you */
                    if (tile_fort(t) && s == 0) v = 150;                                 /* an empty fort */
                    v += p->cell_value[c];
                    if (rnd(p) < OPPV(p, 0xA4F0)) v -= rnd(p);
                    if (v > best) { best = v; p->target_cell = c; }
                }
            }
            if (p->target_cell == -1) p->target_cell = flag_c;
        } else if (rnd(p) < 25) {
            p->ai_hold = false;
        }
        p->ai_dest = q->dest = p->target_cell;
    }
    if (ai_route(p)) q->planned = true;
}

/* ovl6:7A94: a squad on a route stops at the end of a cell: grab a flag
 * nearby, attack a squad nearby, or march the next cell of the route */
static void ai_follow_route(Play *p)
{
    int i = p->ai_sq;
    PSquad *q = &p->sq[i];
    int tx, ty, here = 0, dest = 0, code;
    int f = ai_nearest_flag(p, true, 30);
    if (f != -1) {
        int c = flag_cell(p, f);
        int at = ai_count_stopped(p, c, true);
        if (at <= 2 && rnd(p) < OPPV(p, 0xA4F6) && !(tile_fort(tile_of_cell(p, c)) && !q->archers && at != 0))
            goto grab_flag;
    }
    here = cell_of(q);
    int j = ai_nearest_squad(p, true, here, 2);
    if (OPPV(p, 0xA4F2) >= p->ai_dist && (q->archers || !tile_fort(tile_of_cell(p, cell_of(&p->sq[j]))))) {
        p->ai_dist <<= 2;                              /* attack */
        tx = p->sq[j].x;
        ty = p->sq[j].y;
        goto go;
    }
    p->ai_dist = 5;
    code = p->route[q->route_pos];
again:
    p->route[q->route_pos] = 0;
    if (code != PATH_END && code != 0) {               /* the next cell of the route */
        int c = here + CELL_STEP[code >= 1 && code <= 8 ? code : 0];
        p->ai_dist = 30;
        if (!ai_make_path(p, cell_x(c), cell_y(c), 30)) p->route[q->route_pos] = (int8_t)code;
        else q->route_pos++;
        return;
    }
    dest = q->dest;
    if (here != dest) {                                /* plan the rest */
        p->ai_dest = dest;
        if (ai_route(p)) {
            code = p->route[q->route_pos];
            if (code != PATH_END && code != 0) goto again;
        }
    }
    q->moving = 0;                                     /* arrived */
    q->on_route = 0;
    q->planned = false;
    if (here == p->target_cell) p->target_cell = -1;
    ai_join(p);
    ai_join(p);
    f = ai_nearest_flag(p, false, 4);                  /* stand on our flag if it is close */
    if (f == -1 || here != dest || p->ai_dist <= 1) return;
grab_flag:
    tx = p->flag[f].x;
    ty = p->flag[f].y;
    p->ai_dist = 35;
go:
    clear_path(p, i);
    p->route[q->route_pos] = PATH_END;
    {
        int spd = OPPV(p, 0xA4FE);
        if (!ai_make_path(p, tx, ty, p->ai_dist)) spd = 0;
        q->speed = spd;
        q->on_route = spd;
    }
}

/* ovl6:7698: each tick, the next enemy squad standing still gets its orders */
static void ai_tick(Play *p)
{
    int start = p->ai_next;
    if (++p->ai_next > MAX_SQUADS - 1) p->ai_next = 0;
    while (p->ai_next != start) {
        const PSquad *q = &p->sq[p->ai_next];
        if (q->used && !q->friendly && q->moving == 0) break;
        if (++p->ai_next > MAX_SQUADS - 1) p->ai_next = 0;
    }
    PSquad *q = &p->sq[p->ai_next];
    if (!q->used || q->moving || q->fighting || q->friendly) return;
    p->ai_sq = p->ai_next;
    if (q->on_route) {
        if (p->mode != PM_TRACE) ai_follow_route(p);
    } else if (!q->planned) {
        ai_decide(p);
    }
}

/* ovl6:3ADE count_flags: 1 if you have no flags, 2 if the enemy has none.
 * The enemy guards one of its flags, one in a fort if it can. */
static int count_flags(Play *p)
{
    int white = 0, black = 0;
    for (int f = 0; f < MAX_FLAGS; f++) {
        if (!p->flag[f].present) continue;
        if (p->flag[f].white) { white++; continue; }
        black++;
        bool take = tile_fort(tile_of_cell(p, p->home_cell)) ? p->home_flag == -1
                  : (tile_fort(tile_of_cell(p, flag_cell(p, f))) || p->home_flag == -1);
        if (take) { p->home_flag = f; p->home_cell = flag_cell(p, f); }
    }
    return white == 0 ? 1 : black == 0 ? 2 : 0;
}

/* the enemy gets ready at the start of a game (campaign_play, ovl6:3144) */
static void ai_start(Play *p)
{
    p->target_cell = -1;               /* ovl1:33E1 */
    p->attack_flag = -1;
    ai_cell_values(p);
    ai_choose_attack_flag(p);
    p->home_flag = -1;
    count_flags(p);
    for (int k = 0; k < 5; k++) ai_tick(p);
}

/* check_victory (ovl6:36C7, 37F2): a general losing badly attacks all out */
static void ai_all_out(Play *p)
{
    OPP(p, 0xA4E2) = 200;
    OPP(p, 0xA4E8) = 0;
    OPP(p, 0xA4F4) = -100;
}

void play_enemy_beaten(Play *p, int en)
{
    if (rnd(p) > OPPV(p, 0xA4E2) && cell_of(&p->sq[en]) == p->target_cell) p->target_cell = -1;
}

/* ovl6:52F8: every 16 ticks, food from the supply lines and condition from
 * marching, resting and hunger. */
static void food_and_condition(Play *p)
{
    int shape = rule(p, RULE_MEN_SHAPE);
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used) continue;
        int moving = q->moving;
        int d = supply_distance(p, i);
        if (d > 20) q->food--;
        if (d > 40) q->food--;
        if (d < 10) {
            q->food++;
            if (!q->friendly) q->food++;
        }
        int t = tile_at_cell(p, cell_of(q));
        if ((tile_village(t) && rule(p, RULE_VILLAGES_SUPPLY_FOOD)) || (tile_fort(t) && rule(p, RULE_FORTS_SUPPLY_FOOD)))
            q->food = 49;
        q->food = SDL_clamp(q->food, 1, 49);
        if (q->friendly) {
            if (moving == 1) q->condition -= 2 * shape;   /* fast march */
            if (moving == 2) q->condition -= shape;
            if (moving == 0) q->condition++;
            if (q->food < 10) q->condition -= 3;
        } else {
            if (moving == 1 || moving == 2) q->condition--;
            if (moving == 0) q->condition++;
            if (q->food < 10) q->condition -= 2;
        }
        if ((tile_village(t) || tile_fort(t)) && q->food > 10) q->condition++;
        q->condition = SDL_clamp(q->condition, 1, 49);
        if (moving == 1 && q->condition < 15) q->speed = q->moving = 2;   /* too tired */
        if (moving == 2 && q->condition < 5) q->speed = q->moving = 3;
    }
}

/* ovl6:5526: every 16 ticks, a fort may train a man for a squad in it. */
static void forts_train(Play *p)
{
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        if (!tile_fort(tile_at_cell(p, c))) continue;
        for (int i = 0; i < MAX_SQUADS; i++) {
            PSquad *q = &p->sq[i];
            if (!q->used || cell_of(q) != c) continue;
            if (q->men < 14) {
                if (rnd(p) >= 90) continue;
                if (rnd(p) < rule(p, RULE_FORTS_TRAIN_MEN)) {
                    int r = rnd(p);
                    if (r < 42) q->archers++;
                    else if (r < 84) q->knights++;
                    else q->barbarians++;
                }
                break;
            }
            if (!q->friendly && q->moving == 0) enemy_split_in_fort(p, i);   /* a full enemy squad splits */
        }
    }
    count_men(p);
}

static void alert_flag(Play *p, int f, bool taken)     /* ovl6:5BE7, ovl6:5C56 */
{
    int w = where_code(p->flag[f].x, p->flag[f].y) + 10;
    if (w == p->alert_where) return;
    p->alert_where = w;
    message(p);
    print(p, O5(p, taken ? 0x147 : 0x15B));    /* URGENT - FLAG TAKEN / ENEMY FLAG CAPTURED */
    print_where(p, p->flag[f].x, p->flag[f].y);
}

/* ovl6:392B, ovl6:3ADE: a squad standing on the other side's flag takes it.
 * Returns 1 if you have no flags left, 2 if the enemy has none. */
static int flags_check(Play *p)
{
    bool rechoose = false;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->fighting) continue;
        for (int f = 0; f < MAX_FLAGS; f++) {
            PFlag *fl = &p->flag[f];
            if (!fl->present || fl->white == q->friendly) continue;
            int dx = q->x - fl->x, dy = q->y - fl->y;
            if (dx < -1 || dx > 1 || dy < -1 || dy > 2) continue;
            if (fl->white) {
                p->flags_taken++;
                alert_flag(p, f, true);
                if (f == p->attack_flag || p->attack_flag == -1) rechoose = true;   /* the enemy needs a new target */
            } else {
                p->flags_captured++;
                alert_flag(p, f, false);
                if (f == p->home_flag) p->home_flag = -1;
            }
            fl->white = q->friendly;
            ai_cell_values(p);
            break;
        }
    }
    if (rechoose) ai_choose_attack_flag(p);
    return count_flags(p);
}

/* ovl1:3505: a side with no flag gets one somewhere in the middle of the map. */
static void place_flag(Play *p, int f, bool white)
{
    p->flag[f].present = true;
    p->flag[f].white = white;
    p->flag[f].x = rnd(p) / 2 + 8;
    p->flag[f].y = rnd(p) + 31;
}

/* ------------------------------------------------------------------ View */

static void show_view(Play *p)         /* ovl6:89A9 */
{
    supply_map(p);
    text_reset(p);
    clear_row(p, 24);
    print(p, DS(p, 0x29C));             /* End */
    int mine = 0, theirs = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        if (!p->sq[i].used) continue;
        if (p->sq[i].friendly) mine += p->sq[i].men;
        else theirs += p->sq[i].men;
    }
    tpos(p, 23, 20);
    print(p, DS(p, 0x2A0)); print_num(p, theirs); print(p, DS(p, 0x2AB));   /* Enemy has N men */
    tpos(p, 24, 20);
    print(p, DS(p, 0x2B0)); print_num(p, mine); print(p, DS(p, 0x2BA));     /* you have N men */
    p->mode = PM_VIEW;
}

/* ------------------------------------------------------------------ cursor and selection */

static bool scroll_up_key(int k)   { return k == PK_MINUS || k == PK_KP_MINUS; }   /* rt:0FA2 (and Tab) */
static bool scroll_down_key(int k) { return k == PK_EQUALS || k == PK_KP_PLUS; }   /* rt:0FF5 (and Ctrl) */
static bool cursor_key(int k)      { return k > 0x46 && k < 0x52 && !scroll_up_key(k) && !scroll_down_key(k); }

static bool visible(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    return q->used && (q->friendly || p->reveal || rule(p, RULE_ENEMY_SEEN) >= RULE_SEEN_ALL || q->seen);
}

/* ovl6:6171: is the cursor over a squad? */
static void check_over(Play *p, int key)
{
    if ((scroll_up_key(key) || scroll_down_key(key)) && p->mode != PM_SQUAD) return;
    if (p->mode == PM_TRACE || p->mode == PM_PROMPT) return;
    bool found = false;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (!visible(p, i)) continue;
        if (p->cx != q->x + 1 || p->cy < q->y - 1 || p->cy > q->y + 3) continue;
        found = true;
        p->sel = i;
        if (!p->over_squad) { snd_buzz(); cmd_squad(p); }   /* ovl6:6266 */
        if (q->fighting) break;
    }
    if (!found && p->over_squad) cmd_normal(p);
    p->over_squad = found;
}

static void trace_start(Play *p)       /* ovl6:477D, ovl6:6E1A */
{
    snd_beep();                         /* ovl6:3E05 */
    PSquad *q = &p->sq[p->sel];
    if (q->has_path) stop_squad(p, p->sel);
    int start = free_run(p);
    if (start < 0) return;              /* the path memory is full */
    p->path_next = start;
    q->path_pos = start;
    p->trace_start = start;
    text_reset(p);
    tpos(p, 23, 0);
    print(p, O5(p, 0x209));             /* Trace path to destination and press [M] */
    clear_row(p, 24);
    print(p, O5(p, 0x22E));             /* Mark Cancel */
    p->mode = PM_TRACE;
    p->cx = q->x;
    p->cy = q->y;
}

static void trace_cancel(Play *p)
{
    snd_beep();                         /* ovl6:3E2D */
    stop_squad(p, p->sel);
    /* WAR.EXE leaves the cancelled steps in the path memory; free them */
    for (int k = p->trace_start; k != p->path_next; ) {
        k = next_slot(k);
        p->path[k] = 0;
    }
    cmd_normal(p);
}

static void trace_confirm(Play *p)     /* ovl6:48AA */
{
    snd_beep();                         /* ovl6:48B0 */
    PSquad *q = &p->sq[p->sel];
    if (q->path_pos == p->path_next) {  /* no steps: same as Cancel */
        stop_squad(p, p->sel);
        p->mode = PM_NORMAL;
        cmd_normal(p);
        return;
    }
    p->cx += 1;
    p->cy += 3;
    p->mode = PM_NORMAL;
    q->has_path = true;
    q->marked = true;
    q->mark_x = p->cx;
    q->mark_y = p->cy + 1;
    p->path_next = next_slot(p->path_next);
    if (p->path[p->path_next] != 0) p->path_next = prev_slot(p->path_next);
    p->path[p->path_next] = PATH_END;
    p->path_next = next_slot(p->path_next);
    q->path_pos = next_slot(q->path_pos);
    q->moving = q->speed;
    q->countdown = q->speed;
    cmd_normal(p);
}

/* ovl6:412B: store one move of the box in the path memory. */
static void trace_store(Play *p, int delta)
{
    for (;;) {
        p->path_next = next_slot(p->path_next);
        if (p->path[p->path_next] != 0) {   /* full: mark the destination here */
            p->path_next = prev_slot(p->path_next);
            p->path[p->path_next] = 0;
            p->path_next = prev_slot(p->path_next);
            trace_confirm(p);
            return;
        }
        if (delta > 120)       { delta -= 80; p->path[p->path_next] = 80; }
        else if (delta < -120) { delta += 80; p->path[p->path_next] = -80; }
        else                   { p->path[p->path_next] = (int8_t)delta; return; }
    }
}

/* ovl6:3E56-41AC: move the cursor or scroll the view. */
static void cursor_input(Play *p, int key)
{
    int maxx = CX_MAX, maxy = CY_MAX;
    if (p->mode == PM_TRACE) {
        p->trace_toggle = !p->trace_toggle;
        maxx--; maxy -= 2;
    } else {
        p->trace_toggle = false;
    }
    if (p->trace_toggle) return;        /* the box moves at half speed */
    bool up = scroll_up_key(key), down = scroll_down_key(key);
    if (!up && !down && !cursor_key(key)) return;

    int delta = 0;
    if (up) {
        if (p->view_top < 2) { if (p->demo) p->demo_key = 0; return; }   /* ovl6:3EF8 */
        if (p->jump && p->mode != PM_TRACE) {
            p->view_top = 0; p->cx = 0x26; p->cy = 0x2D; delta = 1;
        } else {
            p->view_top -= 2;
            if (p->view_top + VIEW_MARGIN < p->cy) { p->cy -= 2; delta = -160; }
        }
    } else if (down) {
        if (p->view_top >= VIEW_TOP_MAX - 1) { if (p->demo) p->demo_key = 0; return; }   /* ovl6:3F8E */
        if (p->jump && p->mode != PM_TRACE) {
            p->view_top = VIEW_TOP_MAX; p->cx = 0x26; p->cy = 0x91; delta = 1;
        } else {
            p->view_top += 2;
            if (p->cy < p->view_top) { p->cy += 2; delta = 160; }
        }
    } else {
        if (p->mode == PM_PROMPT) return;
        int k = key - PK_HOME;
        p->cx += STEP_DX[k];
        p->cy += STEP_DY[k];
        delta = STEP_DY[k] * 80 + STEP_DX[k];
    }
    if (p->cx > maxx) { p->cx--; delta--; }
    if (p->cx < 1)    { p->cx++; delta++; }
    if (p->cy > maxy) { p->cy--; delta -= 80; }
    if (p->cy < 0)    { p->cy++; delta += 80; }
    if (p->cy < p->view_top) p->view_top--;
    else if (p->view_top + VIEW_MARGIN < p->cy) p->view_top++;
    p->view_top = SDL_clamp(p->view_top, 0, VIEW_TOP_MAX);
    if (delta == 0) return;
    if (p->mode != PM_TRACE) check_over(p, key);
    else trace_store(p, delta);
}

/* ------------------------------------------------------------------ Size and Info */

static void show_size(Play *p)         /* ovl6:6EA0 */
{
    snd_beep();                         /* ovl6:6EA3 */
    const PSquad *q = &p->sq[p->sel];
    text_reset(p);
    if (q->knights)    { print(p, O5(p, 0x2DF)); print_num(p, q->knights); spaces(p, 1); }
    if (q->barbarians) { print(p, O5(p, 0x2E8)); print_num(p, q->barbarians); spaces(p, 1); }
    if (q->archers)    { print(p, O5(p, 0x2F4)); print_num(p, q->archers); spaces(p, 1); }
    if (q->spies)      { print(p, DS(p, 0x267)); print_num(p, q->spies); }
    clear_row(p, 24);
    print(p, DS(p, 0x26C));
    p->mode = PM_SIZE;
}

static void info_orders(Play *p)       /* ovl6:7245: row 18 */
{
    const PSquad *q = &p->sq[p->sel];
    clear_row(p, 18);
    int sp = q->speed;
    if (q->moving == 0) {
        if (!q->friendly) {
            print(p, O5(p, 0x2FD));     /* NOT MOVING */
        } else {
            print(p, O5(p, 0x23A));     /* STOPPED - when moving: */
            print(p, O5(p, sp == 3 ? 0x308 : sp == 2 ? 0x313 : 0x31E));
        }
        return;
    }
    print(p, O5(p, sp == 3 ? 0x329 : sp == 2 ? 0x334 : 0x33D));
    print(p, DS(p, 0x27B));             /* " in a " */
    int ew = 0, ns = 0;
    for (int k = 0; k < 3; k++) {       /* the direction of the next steps */
        int v = p->path[(q->path_pos - 1 + k + PATH_SIZE) % PATH_SIZE];
        if (v > 78 && v < 82)        { ns = 1; v -= 80; }
        else if (v < -78 && v > -82) { ns = -1; v += 80; }
        if (v == 1) ew = 1;
        else if (v == -1) ew = -1;
    }
    if (ns < 0) print(p, DS(p, 0x282));
    else if (ns > 0) print(p, DS(p, 0x288));
    if (ew < 0) print(p, DS(p, 0x28E));
    else if (ew > 0) {
        if (!ns) print(p, DS(p, 0x293));   /* "\bn " = "an" */
        print(p, DS(p, 0x297));
    }
    if (!ns && !ew) print(p, O5(p, 0x348));   /* "\bn unknown" */
    print(p, O5(p, 0x353));                   /* " direction" */
}

static void info_condition(Play *p)    /* ovl6:741F: row 20, bar drawn in play_draw */
{
    const PSquad *q = &p->sq[p->sel];
    clear_row(p, 20);
    print(p, O5(p, 0x252));
    p->cond_mark = 0;
    if (q->moving == 1 || q->moving == 2 || q->food < 10) p->cond_mark = '-';
    else if (q->moving == 0) p->cond_mark = '+';
}

static void info_food(Play *p)         /* ovl6:750F: row 21 */
{
    clear_row(p, 21);
    print(p, O5(p, 0x261));
    int d = supply_distance(p, p->sel);
    p->food_mark = d > 20 ? '-' : d < 10 ? '+' : 0;
}

static void show_info(Play *p)         /* ovl6:6FAB */
{
    snd_beep();                         /* ovl6:6FBF */
    text_reset(p);
    text_rows_add(p, 5);
    info_orders(p);
    info_condition(p);
    info_food(p);
    p->mode = PM_INFO;
    clear_row(p, 24);
    if (p->sq[p->sel].friendly) print(p, DS(p, 0x270));   /* Speed */
    print(p, DS(p, 0x277));                                /* End */
}

static void info_speed(Play *p)        /* ovl6:7072: S steps slow, march, fast, stop */
{
    snd_beep();                         /* ovl6:706F */
    PSquad *q = &p->sq[p->sel];
    if (!(q->has_path && q->moving == 0) && --q->speed < 0) q->speed = 3;
    if (q->condition < 15 && q->speed == 1) q->speed = 0;   /* too tired to march fast */
    if (q->condition < 5 && q->speed == 2) q->speed = 0;    /* or to march */
    if (q->has_path) q->moving = q->speed;                  /* 0 halts a moving squad */
    if (q->speed == 0) q->speed = 3;
    info_orders(p);
    info_condition(p);
}

/* ------------------------------------------------------------------ public */

int  play_rnd(Play *p) { return rnd(p); }
int  play_edge(const Play *p, int i, int j) { return edge(p, i, j); }
int  play_tile_under(const Play *p, int i) { return tile_at_cell(p, cell_of(&p->sq[i])); }
void play_clear_path(Play *p, int i) { clear_path(p, i); }

/* campaign_play after the battle overlay: squads with no men go, the
 * battle's squads carry on along their paths (ovl6:3589) */
void play_after_battle(Play *p, int a, int b)
{
    p->zoom_squad = -1;
    p->sq[a].clock = p->sq[b].clock = 0;
    p->sq[a].fighting = p->sq[b].fighting = false;
    squads_update(p);
    p->mode = PM_NORMAL;
    p->over_squad = false;
    cmd_normal(p);
    ai_cell_costs(p);                   /* campaign_play starts again (ovl6:3144) */
    supply_map(p);
    if (check_victory(p, a, b)) return;
    ai_cell_values(p);
    check_over(p, 0);
}

void play_resume(Play *p)
{
    p->result = PLAY_RUNNING;
    p->key = 0;
    squads_update(p);
    p->mode = PM_NORMAL;
    p->over_squad = false;
    text_reset(p);
    cmd_normal(p);
    ai_cell_costs(p);
    supply_map(p);
    if (check_victory(p, -1, -1)) return;
    ai_cell_values(p);
    check_over(p, 0);
}

/* The CGA map: terrain is read from its pixels, whichever colours are shown */
static void build_terrain(Play *p)
{
    for (int row = 0; row < MAP_H; row++)
        for (int col = 0; col < MAP_W; col++) {
            int t = p->c->map[row][col] - 1;
            const Image *tile = (t >= 0 && t < NUM_TILES) ? &p->ga->set[MODE_CGA].tiles[t] : NULL;
            for (int y = 0; y < TILE_H; y++)
                for (int x = 0; x < TILE_W; x++)
                    p->terrain[row * TILE_H + y][col * TILE_W + x] =
                        (tile && tile->px) ? (tile->px[y * TILE_W + x] & 3) : 0;
        }
}

void play_prepare(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed)
{
    SDL_memset(p, 0, sizeof *p);
    p->c = c;
    p->ga = ga;
    p->rnd = (uint16_t)(seed & 0xFF);
    p->rnd_ptr = 0xBB8;
    build_terrain(p);
    for (int r = 0; r < 25; r++) clear_row(p, r);
    p->view_top = VIEW_TOP_MAX;
    p->cx = 0x26;
    p->cy = 0x91;
    p->time_div = 64;
    p->text_rows = 2;
    p->sel = -1;
    p->result = PLAY_RUNNING;
    p->fight_delay = 400;
    p->zoom_squad = -1;
    p->target_cell = p->attack_flag = -1;
}

static void start_game(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed, bool demo)
{
    SDL_memset(p, 0, sizeof *p);
    p->demo = demo;
    p->c = c;
    p->ga = ga;
    p->rnd = (uint16_t)(seed & 0xFF);   /* rt:2096: BIOS tick count */
    p->rnd_ptr = 0xBB8;

    const uint8_t *a = c->armies;
#define W(off, i) ((int16_t)(a[(off) + 2 * (i)] | (a[(off) + 2 * (i) + 1] << 8)))
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        int slot = W(80, i);
        q->used = slot != 9999;
        q->moving = q->used ? slot : 0;
        q->friendly = W(0, i) == -1;
        q->speed = W(40, i);
        q->x = W(120, i);
        q->y = W(160, i);
        q->formation = a[200 + i];
        q->condition = a[220 + i];
        q->food = a[240 + i];
        q->archers = a[320 + i];
        q->knights = a[340 + i];
        q->barbarians = a[360 + i];
        q->spies = a[380 + i];
        q->men = q->archers + q->knights + q->barbarians;
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        p->flag[i].present = W(260, i) >= 0;
        p->flag[i].x = W(260, i);
        p->flag[i].y = W(280, i);
        p->flag[i].white = W(300, i) == -1;
    }
#undef W

    build_terrain(p);

    for (int r = 0; r < 25; r++) clear_row(p, r);
    p->view_top = VIEW_TOP_MAX;         /* ovl1:32B8 */
    p->cx = 0x26;
    p->cy = 0x91;
    p->time_div = demo ? 1 : 64;        /* slow; the demo runs faster (ovl1:33C9) */
    p->text_rows = 2;
    p->sel = -1;
    p->result = PLAY_RUNNING;
    p->fight_delay = 400;               /* Options Shoppe: medium (ovl1:3114) */
    p->zoom_squad = -1;

    /* the opponent's values (choose_opponent, ovl1:4CD4) */
    if (ga->have_o2) {
        SDL_memcpy(p->opp, ga->opponent[c->opponent & 7], sizeof p->opp);
    } else {                            /* no O2: no surrender offers, rules for terrain */
        OPP(p, 0xA4EA) = 500;
        OPP(p, 0xA4FA) = rule(p, RULE_MOUNTAIN);
        OPP(p, 0xA4FC) = rule(p, RULE_FOREST);
    }
    if (demo) OPP(p, 0xA4FE) = 3;       /* ovl1:4382 */

    /* game_init (ovl1:32E5): when every squad is seen, spies are no use and
     * become barbarians; the opponent guards its flag with at most half its squads */
    int enemies = 0;
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        if (rule(p, RULE_ENEMY_SEEN) >= RULE_SEEN_ALL) { q->barbarians += q->spies; q->spies = 0; }
        if (!q->friendly && q->used) enemies++;
    }
    if (OPP(p, 0xA4E8) > enemies >> 1) OPP(p, 0xA4E8) = (int16_t)(enemies >> 1);
    /* game_init (ovl1:3417): a side without a flag gets one */
    int white = 0, black = 0;
    for (int f = 0; f < MAX_FLAGS; f++)
        if (p->flag[f].present) { if (p->flag[f].white) white++; else black++; }
    if (!white) {
        int f = 0;
        while (f < 9 && p->flag[f].present) f++;
        place_flag(p, f, true);
    }
    if (!black) {
        int f = 0;
        while (f < 8 && p->flag[f].present) f++;
        place_flag(p, f, false);
    }

    squads_update(p);
    ai_cell_costs(p);                   /* campaign_play: ovl6:3144 */
    supply_map(p);
    update_delays(p, false);
    cmd_normal(p);
    if (check_victory(p, -1, -1)) { check_over(p, 0); return; }
    ai_start(p);                        /* a new game: the enemy's first orders */
    if (demo) p->demo_key = PK_KP_MINUS;   /* ovl6:3197: the demo scrolls up the map first */
    snd_new_game();                     /* ovl6:317D, 3194, 31FE */
    check_over(p, 0);
}

void play_start(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed)
{
    start_game(p, c, ga, seed, false);
}

void play_start_demo(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed)
{
    start_game(p, c, ga, seed, true);
}

void play_key(Play *p, int key, bool shift)
{
    p->jump = shift;
    p->key = key;
}

/* ------------------------------------------------------------------ Detach, Join, Formation */

/* The four kinds of men, in the order the Detach/Join table lists them
 * (archers, knights, barbarians, spies; labels O5 via DS:B2E). */
static int *men_of(PSquad *q, int k)
{
    return k == 0 ? &q->archers : k == 1 ? &q->knights : k == 2 ? &q->barbarians : &q->spies;
}
static const unsigned GROUP_LABEL[4] = { 0x18D, 0x16F, 0x17E, 0x19C };   /* -- ARCHERS --- ... */

static void group_show(Play *p)        /* ovl6:64E2: the table, the arrow and the keys */
{
    PSquad *a = &p->sq[p->sel], *b = &p->sq[p->partner];
    count_men(p);
    if (a->men >= 14 && p->g_join) { p->g_join = false; snd_group_full(); }   /* ovl6:6502: it can't take more */
    if (b->men >= 14 && !p->g_join) { p->g_join = true; snd_group_full(); }
    tpos(p, p->g_top - 1, 17);
    print(p, DS(p, p->g_join ? 0x1FF : 0x203));             /* <-- or --> */
    int row = p->g_top;
    for (int k = 0; k < 4; k++) {
        int na = *men_of(a, k), nb = *men_of(b, k);
        if (!na && !nb) continue;
        tpos(p, row, 8);
        if (na < 10) print(p, " ");
        print_num(p, na);
        tpos(p, row, 28);
        if (nb < 10) print(p, " ");
        print_num(p, nb);
        row++;
    }
    clear_row(p, 23);
    print(p, O5(p, 0x1E7));                                 /* Select from the above types to */
    print(p, O5(p, p->g_join ? 0x2AF : 0x2B7));             /* Join / Detach */
    clear_row(p, 24);
    print(p, DS(p, 0x207));                                 /* End */
    static const int ORDER[4] = { 1, 2, 0, 3 };             /* Knight, Barb., Archer, Spy */
    static const unsigned KEYS[4] = { 0x20C, 0x214, 0x21B, 0x223 };
    for (int n = 0; n < 4; n++) {
        int k = ORDER[n];
        if ((*men_of(a, k) && !p->g_join) || (*men_of(b, k) && p->g_join)) print(p, DS(p, KEYS[n]));
    }
    spaces(p, 2);
    if (p->g_join) { if (a->men > 0 && b->men < 14) print(p, O5(p, 0x2B7)); }   /* Detach */
    else if (b->men > 0 && a->men < 14) print(p, O5(p, 0x2AF));                 /* Join */
}

/* ovl6:62A7: Detach (men go to a new squad) or Join (men come from a squad nearby) */
static void group_start(Play *p, bool join)
{
    snd_beep();                         /* ovl6:62AD */
    PSquad *a = &p->sq[p->sel], *b = &p->sq[p->partner];
    count_men(p);
    p->g_top = 22;
    for (int k = 0; k < 4; k++) if (*men_of(a, k) || *men_of(b, k)) p->g_top--;
    text_reset(p);
    text_rows_add(p, 24 - p->g_top);
    int row = p->g_top;
    for (int k = 0; k < 4; k++) {
        if (!*men_of(a, k) && !*men_of(b, k)) continue;
        tpos(p, row++, 12);
        print(p, O5(p, GROUP_LABEL[k]));
    }
    tpos(p, p->g_top - 1, 4);
    if (!join) {
        print(p, O5(p, 0x1AB));                             /* OLD GROUP            NEW GROUP */
        new_squad_from(p, p->sel, p->partner);
    } else {
        print(p, O5(p, 0x1CA));                             /* GROUP 1              GROUP 2 */
    }
    int total = a->men + b->men;                            /* food and condition are shared out */
    if (total > 0) {
        int food = (b->men * b->food + a->men * a->food) / total;
        int cond = (b->men * b->condition + a->men * a->condition) / total;
        a->food = b->food = food;
        a->condition = b->condition = cond;
    }
    p->g_join = join;
    p->mode = PM_GROUP;
    group_show(p);
}

static void group_key(Play *p, int key)
{
    PSquad *a = &p->sq[p->sel], *b = &p->sq[p->partner];
    int k = key == PK_K ? 1 : key == PK_B ? 2 : key == PK_A ? 0 : key == PK_S ? 3 : -1;
    if (k >= 0) {
        int *from = men_of(p->g_join ? b : a, k), *to = men_of(p->g_join ? a : b, k);
        if (!*from) return;
        (*from)--;
        (*to)++;
        snd_beep();                                         /* ovl6:680A, 686C, 68CE, 6930 */
    } else if (key == PK_J) {
        if (b->men == 0 || p->g_join) return;
        p->g_join = true;
        snd_beep();                                         /* ovl6:6958 */
    } else if (key == PK_D) {
        if (a->men == 0 || !p->g_join) return;
        p->g_join = false;
        snd_beep();                                         /* ovl6:697F */
    } else if (key == PK_E) {                               /* End */
        snd_beep();                                         /* ovl6:698A */
        text_reset(p);
        squads_update(p);
        p->over_squad = false;
        if (a->used) cmd_squad(p);
        else cmd_normal(p);
        return;
    } else {
        return;
    }
    group_show(p);
}

/* rt:09D3 place_formation, for the preview: who stands where */
static int formation_places(const Play *p, int i, int type[14], int x[14], int y[14])
{
    const PSquad *q = &p->sq[i];
    const uint8_t *F = p->ga->formations;
    int form = SDL_clamp(q->formation, 0, 9), n = 0;
    int ar = q->archers, ba = q->barbarians, kn = q->knights;
    for (int k = 0; k < 14; k++) {
        int t = F[form * 14 + k];
        if (t == 2 && ar) ar--;
        else if (t == 1 && ba) ba--;
        else if (t == 0 && kn) kn--;
        else if (kn) { kn--; t = 0; }
        else if (ba) { ba--; t = 1; }
        else if (ar) { ar--; t = 2; }
        else break;
        type[n] = t;
        x[n] = F[140 + form * 14 + k] - (t == 2 ? 2 : 0);
        y[n] = F[280 + form * 14 + k];
        n++;
    }
    return n;
}

static void formation_text(Play *p)    /* rt:0EB9: Formation: N */
{
    tpos(p, 23, 21);
    print(p, DS(p, 57));
    print_num(p, p->sq[p->sel].formation);
    print(p, " ");
}

/* ovl6:462B: Formation: + and - choose one of the 9 formations */
static void formation_start(Play *p)
{
    snd_beep();                         /* ovl6:3D2C */
    PSquad *q = &p->sq[p->sel];
    if (q->formation == 0) q->formation = 1;
    p->mode = PM_FORMATION;
    text_reset(p);
    text_rows_add(p, 1);
    clear_row(p, 24);
    tpos(p, 24, 7);
    print(p, DS(p, 0x1D6));                                 /* End */
    formation_text(p);
}

static void formation_key(Play *p, int key)
{
    PSquad *q = &p->sq[p->sel];
    if (key == PK_EQUALS || key == PK_KP_PLUS) {
        if (++q->formation > 9) q->formation = 1;
        snd_beep();                                         /* ovl6:46E9 */
        formation_text(p);
    } else if (key == PK_MINUS || key == PK_KP_MINUS) {
        if (--q->formation < 1) q->formation = 9;
        snd_beep();                                         /* ovl6:4722 */
        formation_text(p);
    } else if (key == PK_E || (key >= PK_HOME && key <= PK_PGDN)) {   /* E, or a cursor key */
        if (key == PK_E) snd_beep();                        /* ovl6:46B5 */
        p->over_squad = false;
        cmd_squad(p);
    }
}

static void handle_key(Play *p, int key)       /* ovl6:3B87 */
{
    if (p->demo == 2) {                 /* ovl6:3B98: the demo zooms to an encounter */
        p->demo_key = key = PK_Z;
        p->mode = PM_SQUAD;
    }
    if (p->mode == PM_PROMPT && p->demo) {   /* ovl6:3BBD: a question, or the end of the game, */
        if (!p->ended || ++p->demo_wait > 3 * PLAY_HZ) p->result = PLAY_QUIT;   /* ends the demo */
        return;
    }
    if (p->mode == PM_PROMPT) {
        if (p->ended) {                 /* the map can still be scrolled */
            if (key == PK_ENTER || key == PK_SPACE) p->result = p->outcome;
            else cursor_input(p, key);
        } else if (p->prompt_surrender) {
            if (key == PK_Y) {
                p->prompt_surrender = false;
                clear_row(p, 24);
                print(p, O5(p, 0x47));  /* Your opponent accepts your surrender */
                p->ended = true;
                p->outcome = PLAY_LOST;
                p->result_code = 11;
            } else if (key == PK_N) {
                p->prompt_surrender = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
            }
        } else if (p->prompt_quit) {    /* the remake's Esc */
            if (key == PK_Y) {
                p->prompt_quit = false;
                p->result = PLAY_QUIT;
            } else if (key == PK_N || key == PK_ESC) {
                p->prompt_quit = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
            }
        } else if (p->prompt_offer) {   /* ovl6:37B6 */
            if (key == PK_Y || key == PK_N) snd_beep();   /* ovl6:37C5, 37E3 */
            if (key == PK_Y) {
                print(p, DS(p, 0x1CF)); /* Yes */
                p->ended = true;
                p->outcome = PLAY_WON;
                p->result_code = 1;
            } else if (key == PK_N) {
                print(p, DS(p, 0x1D3)); /* No */
                ai_all_out(p);          /* ovl6:37F2: refused, the enemy attacks all out */
                p->prompt_offer = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
            }
        }
        return;
    }
    if (p->mode == PM_VIEW) {
        if (key) { snd_beep(); cmd_normal(p); }   /* ovl6:8B77 */
        return;
    }
    if (p->mode == PM_GROUP) { group_key(p, key); return; }
    if (p->mode == PM_FORMATION) { formation_key(p, key); return; }
    if (p->mode == PM_SIZE) {
        if (key) { snd_beep(); cmd_squad(p); }    /* ovl6:6F93 */
        return;
    }
    if (p->mode == PM_INFO) {
        if (key == PK_E) { snd_beep(); cmd_squad(p); }   /* ovl6:7038 */
        else if (key == PK_S && p->sq[p->sel].friendly) info_speed(p);
        return;
    }
    if (p->mode == PM_NORMAL) {
        if (key == PK_ALT_S) { p->result = PLAY_SAVE; return; }   /* save the game (ovl6:3BE5) */
        if (key == PK_ESC) {            /* the remake: leave the game */
            clear_row(p, 23);
            clear_row(p, 24);
            tpos(p, 24, 0);
            print(p, "Quit to the title page? (Y or N)");
            p->mode = PM_PROMPT;
            p->prompt_quit = true;
            return;
        }
        if (key == PK_S) {              /* Surrender */
            snd_beep();                 /* ovl6:3BE2 */
            clear_row(p, 23);
            clear_row(p, 24);
            print(p, O5(p, 0x22));
            p->mode = PM_PROMPT;
            p->prompt_surrender = true;
            return;
        }
        if (key == PK_T) {              /* Time: slow, medium, fast, faster */
            p->time_div >>= 2;
            if (p->time_div < 1) p->time_div = 64;
            snd_beep();                 /* ovl6:3C1E */
            show_time(p);
        } else if (key == PK_C) {       /* Clear the messages */
            if (p->msg_rows) snd_beep();   /* ovl6:3C37 */
            text_reset(p);
            show_clear(p);
        } else if (key == PK_V) {       /* View */
            snd_beep();                 /* ovl6:3C4B */
            show_view(p);
            return;
        }
    } else if (p->mode == PM_SQUAD) {
        const PSquad *q = &p->sq[p->sel];
        if (key == PK_I) { show_info(p); return; }
        if (key == PK_Z && q->fighting && q->clock > 0) { p->zoom_squad = p->sel; return; }   /* Zoom (ovl6:3D97) */
        if (key == PK_S) { show_size(p); return; }
        if (key == PK_M && q->friendly && !q->fighting) { trace_start(p); return; }
        if (key == PK_D && p->free_slot > -1 && q->friendly && !q->fighting) {   /* Detach (ovl6:3C99) */
            p->partner = p->free_slot;
            group_start(p, false);
            return;
        }
        if (key == PK_F && !q->small && q->friendly && !q->fighting) { formation_start(p); return; }   /* ovl6:3CD5 */
        if (key == PK_J && q->friendly) {                                  /* Join (ovl6:3D40) */
            p->partner = join_partner(p, p->sel) - 1;
            if (p->partner > -1 && (p->sq[p->partner].men < 14 || q->men < 14)) { group_start(p, true); return; }
        }
    } else if (p->mode == PM_TRACE) {
        if (key == PK_C) { trace_cancel(p); return; }
        if (key == PK_M) { trace_confirm(p); return; }
    }
    cursor_input(p, key);
}

void play_frame(Play *p, int held)
{
    if (p->result != PLAY_RUNNING) return;
    int key = p->key ? p->key : held;
    p->key = 0;
    if (p->demo) {
        if (key) { p->result = PLAY_QUIT; return; }   /* ovl6:3231: a key ends the demo */
        /* poll_input (ovl6:4241): the demo holds its key, now and then -/+ instead */
        if (rnd(p) <= 3) p->demo_key = p->rnd > 1 ? PK_KP_MINUS : PK_KP_PLUS;
        key = p->demo_key;
    }
    p->tick_count++;
    bool frozen = p->mode == PM_TRACE || p->mode == PM_PROMPT || p->mode == PM_SIZE ||
                  p->mode == PM_INFO || p->mode == PM_VIEW || p->mode == PM_GROUP || p->mode == PM_FORMATION;
    if (key == 0 && !frozen && p->tick_count >= p->time_div) {   /* campaign_play: one tick */
        p->tick_count = 0;
        p->ticks++;
        int r = flags_check(p);
        if (r) { game_over(p, r == 2); p->result_code = r == 1 ? 10 : 0; return; }
        encounters(p);
        if (p->mode != PM_TRACE) ai_tick(p);    /* the enemy's orders */
        move_squads(p);
        if (p->mode == PM_PROMPT) return;       /* lost a squad and the game is over */
        if (p->ticks > 15) {                    /* every 16 ticks */
            if (--p->alert_timer < 0) p->alert_where = 0;
            p->ticks = 0;
            supply_map(p);
            food_and_condition(p);
            forts_train(p);
        }
    }
    if (p->demo) key = p->demo_key;             /* the demo may scroll to a message */
    if ((key == 0 || p->demo) && p->mode != PM_PROMPT && p->mode != PM_SIZE && p->mode != PM_INFO &&
        p->mode != PM_VIEW && p->mode != PM_GROUP && p->mode != PM_FORMATION)
        battles(p);                             /* campaign_play: every iteration no key is down */
    handle_key(p, key);
}

/* ------------------------------------------------------------------ drawing */

typedef struct {
    Screen *s;
    int     top, h;          /* first map line shown, lines shown */
    uint8_t col[4];          /* CGA colours 0-3 in the screen palette */
} View;

static void vpx(const View *v, int x, int line, int c)
{
    int sy = line - v->top;
    if (x < 0 || x >= SCREEN_W || sy < 0 || sy >= v->h) return;
    v->s->px[sy * SCREEN_W + x] = v->col[c];
}

/* rt:18A9: the cursor, a cross, or an open box while tracing. */
static void draw_cursor(const View *v, int cx, int cy, bool box)
{
    int x = cx * 4, l = cy * 2;
    if (box) {
        for (int k = 0; k <= 12; k += 12)
            for (int i = 0; i < 8; i++) vpx(v, x + i, l + k, (i >= 2 && i <= 5) ? 3 : 0);
        static const int side[4] = { 2, 4, 8, 10 };
        for (int k = 0; k < 4; k++) {
            vpx(v, x, l + side[k], 3); vpx(v, x + 1, l + side[k], 3);
            vpx(v, x + 6, l + side[k], 3); vpx(v, x + 7, l + side[k], 3);
        }
        return;
    }
    static const int arm[6] = { 0, 1, 2, 7, 8, 9 };
    for (int k = 0; k < 6; k++)
        for (int i = 0; i < 4; i++) vpx(v, x + i, l + arm[k], i < 2 ? 0 : 3);
    for (int k = 4; k <= 5; k++) {
        static const int8_t bar[12] = { 0, 0, 3, 3, 3, -1, -1, 0, 0, 3, 3, 3 };
        for (int i = 0; i < 12; i++)
            if (bar[i] >= 0) vpx(v, x - 4 + i, l + k, bar[i]);
    }
}

static void draw_mark(const View *v, int mx, int my)   /* rt:1CBC */
{
    int x = mx * 4, l = my * 2;
    for (int k = 0; k < 6; k++)
        for (int i = 0; i < 4; i++)
            if ((k == 2 || k == 3) || i == 1 || i == 2) vpx(v, x + i, l + k, 3);
}

static void draw_ds_icon(Screen *s, const GameAssets *ga, unsigned off, int x, int y, const uint8_t col[4])
{
    for (int r = 0; r < 6; r++)          /* draw_icon_8x6 */
        for (int i = 0; i < 8; i++) {
            unsigned at = off + 2 * r + i / 4;
            if (at >= DS_SIZE) continue;
            int c = (ga->ds[at] >> (6 - 2 * (i & 3))) & 3;
            if (x + i < SCREEN_W && y + r < SCREEN_H) s->px[(y + r) * SCREEN_W + x + i] = col[c];
        }
}

static void vline(Screen *s, int xbyte, int y, int len, const uint8_t col[4])   /* draw_vline: byte 0Fh */
{
    for (int j = 0; j < len; j++)
        for (int i = 0; i < 4; i++)
            if (y + j < SCREEN_H) s->px[(y + j) * SCREEN_W + xbyte * 4 + i] = col[i < 2 ? 0 : 3];
}

static void bar(Screen *s, int y, int len, const uint8_t col[4])    /* clear_box */
{
    screen_fill(s, 0x1C * 4, y, 50 * 4, 8, col[0]);
    screen_fill(s, 0x1C * 4, y, SDL_min(len, 50) * 4, 6, col[1]);
}

static void draw_ds_bitmap(Screen *s, const GameAssets *ga, unsigned off, int x0, int y0, const uint8_t col[4])
{
    int w = 0, h = 0;                    /* draw_bitmap */
    assets_ds_bitmap_px(ga, off, 0, 0, &w, &h);
    for (int y = 0; y < h && y0 + y < SCREEN_H; y++)
        for (int x = 0; x < w && x0 + x < SCREEN_W; x++)
            s->px[(y0 + y) * SCREEN_W + x0 + x] = col[assets_ds_bitmap_px(ga, off, x, y, NULL, NULL)];
}

/* one byte (4 pixels) of a CGA bit pattern */
static void pattern(Screen *s, int x, int y, uint8_t b, const uint8_t col[4])
{
    for (int i = 0; i < 4; i++) s->px[y * SCREEN_W + x + i] = col[(b >> (6 - 2 * i)) & 3];
}

/* ovl6:89A9: a small map of who holds what: white and dotted white for you,
 * black and dotted for the enemy (only when the enemy is always seen) */
static void view_graphics(const Play *p, Screen *s, const uint8_t col[4])
{
    screen_fill(s, 0, 0x6A, 12 * 4, 1, col[3]);          /* draw_hline, rt:1E35 */
    for (int y = 0x6B; y < 0x6A + 0x54; y++) {
        pattern(s, 0, y, 0xF0, col);
        for (int x = 1; x <= 10; x++) pattern(s, x * 4, y, 0x00, col);
        pattern(s, 11 * 4, y, 0x0F, col);
    }
    screen_fill(s, 0, 0x6A + 0x54, 12 * 4, 1, col[3]);
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        int v = p->supply[c];
        uint8_t a = 0x55, b = 0x55;      /* rt:1E67: a on lines 0 and 2, b on 1 and 3 */
        if (rule(p, RULE_ENEMY_SEEN) == RULE_SEEN_ALL) {
            if (v > 25) a = b = 0x00;
            else if (v > 9) a = b = 0x11;
            else if (v > 1) a = 0x11;
        }
        if (v < -25) a = b = 0xFF;
        else if (v < -9) a = b = 0xDD;
        else if (v < -1) a = 0xDD;
        int x = (1 + c % 10) * 4, y = 0x6C + (c / 10) * 4;
        pattern(s, x, y, a, col);
        pattern(s, x, y + 1, b, col);
        pattern(s, x, y + 2, a, col);
        pattern(s, x, y + 3, b, col);
    }
}

static void info_graphics(const Play *p, Screen *s, const uint8_t col[4])   /* ovl6:7127 */
{
    const PSquad *q = &p->sq[p->sel];
    screen_fill(s, 0, 0x9C, SCREEN_W, 1, col[3]);
    bar(s, 0xA0, q->condition, col);
    bar(s, 0xA8, q->food, col);
    screen_fill(s, 0, 0xB0, SCREEN_W, 1, col[3]);
    static const int tick_x[5] = { 0x1C, 0x28, 0x34, 0x40, 0x4C };
    for (int k = 0; k < 5; k++) vline(s, tick_x[k], 0xB0, (k & 1) ? 4 : 6, col);
    static const struct { int x; unsigned off; } label[10] = {
        { 0x1C, 0x3E8 }, { 0x27, 0x400 }, { 0x29, 0x40C }, { 0x33, 0x40C }, { 0x35, 0x3E8 },
        { 0x3F, 0x418 }, { 0x41, 0x40C }, { 0x4A, 0x3F4 }, { 0x4C, 0x3E8 }, { 0x4E, 0x3E8 },
    };   /* 0, 25, 50, 75, 100 */
    for (int k = 0; k < 10; k++) draw_ds_icon(s, p->ga, label[k].off, label[k].x * 4, 0xB8, col);
    uint8_t white = col[3], black = col[0];
    char m[2] = { 0, 0 };
    if (p->cond_mark) { m[0] = p->cond_mark; screen_text(s, p->ga, (q->condition / 2 + 15) * 8, 0xA0, m, white, black); }
    if (p->food_mark) { m[0] = p->food_mark; screen_text(s, p->ga, (q->food / 2 + 15) * 8, 0xA8, m, white, black); }
}

/* rt:0D3F: the formation in a box at the bottom left, the men as A, B and K */
static void formation_graphics(const Play *p, Screen *s, const uint8_t col[4])
{
    for (int y = 86; y < 190; y++) {                       /* rt:1D68 */
        for (int x = 0; x < 68; x++) s->px[y * SCREEN_W + x] = col[x < 2 ? 3 : 0];
    }
    vline(s, 17, 86, 104, col);
    screen_fill(s, 0, 86, 72, 1, col[3]);
    screen_fill(s, 0, 190, 72, 1, col[3]);
    int type[14], x[14], y[14];
    int n = formation_places(p, p->sel, type, x, y);
    for (int k = 0; k < n; k++) {
        int xb = (((x[k] + (type[k] == 2 ? 2 : 0)) >> 1) - 22), line = y[k] + 16;
        xb = SDL_clamp(xb, 1, 15);
        line = SDL_clamp(line, 44, 92) * 2;
        unsigned off = type[k] == 0 ? 0x3DC : type[k] == 1 ? 0x3D0 : 0x3C4;   /* K, B, A */
        for (int r = 0; r < 6; r++) {                      /* draw_icon_8x6 */
            unsigned at = off + r * 2;
            if (at + 1 >= DS_SIZE) break;
            int word = (p->ga->ds[at] << 8) | p->ga->ds[at + 1];
            for (int b = 0; b < 8; b++)
                if (line + r < SCREEN_H) s->px[(line + r) * SCREEN_W + xb * 4 + b] = col[(word >> (14 - 2 * b)) & 3];
        }
    }
    draw_ds_bitmap(s, p->ga, 0x448, 0, 192, col);           /* [+] */
    draw_ds_bitmap(s, p->ga, 0x46C, 6 * 4, 192, col);       /* [-] */
}

void play_draw(const Play *p, Screen *s, GfxMode mode)
{
    const AssetSet *set = &p->ga->set[mode];
    int map_h = SCREEN_H - 8 * p->text_rows - 2;
    campaign_draw_tiles(s, p->ga, mode, p->c, p->view_top * 2, map_h);

    View v = { s, p->view_top * 2, map_h, { 0, 1, 2, 3 } };
    int ncolors = set->tiles[0].ncolors;
    for (int i = 0; i < 4; i++)
        v.col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    uint8_t white = v.col[3], black = v.col[0];

    for (int i = 0; i < MAX_SQUADS; i++)                 /* rt:0F2C: marks, squads, flags, cursor */
        if (p->sq[i].used && p->sq[i].marked && p->sq[i].friendly) draw_mark(&v, p->sq[i].mark_x, p->sq[i].mark_y);
    for (int i = 0; i < MAX_SQUADS; i++) {               /* rt:19D9 */
        const PSquad *q = &p->sq[i];
        if (!visible(p, i)) continue;
        SquadPose pose = q->small ? POSE_SMALL
                       : q->fighting ? POSE_FIGHTING
                       : q->moving == 0 ? POSE_STANDING
                       : q->condition <= 10 ? POSE_WEAK : POSE_MARCHING;
        bool white_fig = q->friendly != (q->clock == FIGHT_ON && p->blink);   /* fighting squads blink */
        screen_sprite(s, p->ga->squad_sprite[pose], q->x * 4, q->y * 2 - v.top, white_fig, white, black, 0, map_h);
    }
    for (int i = 0; i < MAX_FLAGS; i++) {                /* rt:1AE0 */
        const PFlag *f = &p->flag[i];
        if (f->present) screen_sprite(s, p->ga->flag_sprite, f->x * 4, f->y * 2 - v.top, f->white, white, black, 0, map_h);
    }
    if (p->mode != PM_PROMPT) draw_cursor(&v, p->cx, p->cy, p->mode == PM_TRACE);

    /* rt:1CE8: a white line under the map, then the text rows */
    screen_fill(s, 0, map_h, SCREEN_W, 1, white);
    screen_fill(s, 0, map_h + 1, SCREEN_W, SCREEN_H - map_h - 1, black);
    for (int r = 25 - p->text_rows; r < 25; r++)
        screen_text(s, p->ga, 0, r * 8, p->text[r], white, black);

    if (p->mode == PM_TRACE) draw_ds_bitmap(s, p->ga, 0x490, 0x48 * 4, 0xB8, v.col);   /* the [M] key */
    if (p->enter_icon)
        draw_ds_bitmap(s, p->ga, 0x424, 0x1D * 4, 0xC0, v.col);                        /* the [Enter] key */
    if (p->mode == PM_INFO) info_graphics(p, s, v.col);
    if (p->mode == PM_FORMATION) formation_graphics(p, s, v.col);
    if (p->mode == PM_VIEW) view_graphics(p, s, v.col);
    if (p->prompt_offer) {              /* ovl6:3740: a box at the top of the screen */
        for (int r = 3; r <= 6; r++) screen_text(s, p->ga, 0, r * 8, p->text[r], white, black);
        vline(s, 0, 0x18, 0x22, v.col);
        vline(s, 0x4F, 0x18, 0x22, v.col);
        screen_fill(s, 4, 0x18, 0x4F * 4, 1, white);
        screen_fill(s, 4, 0x38, 0x4F * 4, 1, white);
    }
}
