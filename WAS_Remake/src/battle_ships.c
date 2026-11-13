/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle_ships.c: the ships in the sea battle: drawing them (ovl5_afc6),
 * sailing (ovl5_b4c5, b841), turning (rt_59ec), collisions and grappling
 * (ovl5_bc12), the guns (ovl5_936d, the hits abb6, the explosion a7ad), the
 * speeds (ovl6_f0d2) and the ships' orders and aim (rt_34bf-4b35), for your
 * ships as well as the enemy's.
 *
 * Where WAS.EXE looks at a ship with no target (index -1) or one past the
 * last, it reads the neighbouring arrays: vbyte() and vword() rebuild those
 * bytes from the remake's arrays, laid out as in WAS.EXE's data segment.
 */
#include "battle_int.h"

/* ------------------------------------------------------------------ WAS.EXE's layout of the ship arrays */

typedef struct { int base, size; size_t off; } Field;
#define F(name) offsetof(Battle, name)
static const Field FIELDS[] = {
    { 0x69E1, 1, F(type) }, { 0x69E7, 2, F(x) }, { 0x69F3, 2, F(y) }, { 0x69FF, 2, F(owner) },
    { 0x6A0B, 1, F(sails) }, { 0x6A11, 1, F(hull) }, { 0x6A17, 2, F(order) }, { 0x6A23, 2, F(sail_set) },
    { 0x6A2F, 2, F(xcount) }, { 0x6A3B, 2, F(ycount) }, { 0x6A47, 2, F(speed) }, { 0x6A53, 2, F(speed_wait) },
    { 0x6A5F, 2, F(turn_wait) }, { 0x6A6B, 2, F(moved) }, { 0x6A77, 2, F(aim_r) }, { 0x6A83, 2, F(aim_r_set) },
    { 0x6A8F, 2, F(aim_l) }, { 0x6A9B, 2, F(aim_l_set) }, { 0x6AA7, 2, F(crew_side) }, { 0x6AB3, 2, F(gun_l) },
    { 0x6ABF, 2, F(gun_r) }, { 0x6ACB, 2, F(heading) }, { 0x6AD7, 2, F(turn) }, { 0x6AE3, 2, F(turning) },
    { 0x6AEF, 1, F(half) }, { 0x6AF5, 1, F(sub) }, { 0x6AFB, 1, F(target) }, { 0x6B01, 2, F(anim_from) },
    { 0x6B0D, 1, F(draw_order) }, { 0x6B13, 1, F(grappled) },
};
#undef F

static int vbyte(const Battle *b, int addr)
{
    for (size_t k = 0; k < sizeof FIELDS / sizeof FIELDS[0]; k++) {
        const Field *f = &FIELDS[k];
        int end = f->base + f->size * BS;
        if (addr < f->base || addr >= end) continue;
        const int *arr = (const int *)((const char *)b + f->off);
        int i = (addr - f->base) / f->size, v = arr[i];
        if (f->size == 2 && ((addr - f->base) & 1)) v >>= 8;
        return v & 0xFF;
    }
    return mb(b, addr);
}
static int vword(const Battle *b, int addr) { return (int16_t)(vbyte(b, addr) | (vbyte(b, addr + 1) << 8)); }

static int X(const Battle *b, int i)     { return i >= 0 && i < BS ? b->x[i] : vword(b, 0x69E7 + 2 * i); }
static int Y(const Battle *b, int i)     { return i >= 0 && i < BS ? b->y[i] : vword(b, 0x69F3 + 2 * i); }
static int OWN(const Battle *b, int i)   { return i >= 0 && i < BS ? b->owner[i] : vword(b, 0x69FF + 2 * i); }
static int HULL(const Battle *b, int i)  { return i >= 0 && i < BS ? b->hull[i] : (int8_t)vbyte(b, 0x6A11 + i); }
static int SAILS(const Battle *b, int i) { return i >= 0 && i < BS ? b->sails[i] : (int8_t)vbyte(b, 0x6A0B + i); }
static int TYPE(const Battle *b, int i)  { return i >= 0 && i < BS ? b->type[i] : (int8_t)vbyte(b, 0x69E1 + i); }
static int HEAD(const Battle *b, int i)  { return i >= 0 && i < BS ? b->heading[i] : vword(b, 0x6ACB + 2 * i); }

static int wrap16(int h) { while (h > 15) h -= 16; while (h < 0) h += 16; return h; }
static int F42(const Battle *b, int h) { return msb(b, 0xF42 + h); }
static int F52(const Battle *b, int h) { return msb(b, 0xF52 + h); }
static int FAA(const Battle *b, int i) { return mw(b, 0xFAA + 2 * i); }

/* ------------------------------------------------------------------ small rt and ovl6 routines */

/* rt_4fd1 / 5043 / 50b5 / 5127: is the ship heading north / south / east / west
 * (wide: the wider arc) */
static bool head_n(const Battle *b, int s, int wide) { int h = HEAD(b, s); return wide ? (h >= 13 || h <= 3) : (h >= 14 || h <= 2); }
static bool head_s(const Battle *b, int s, int wide) { int h = HEAD(b, s); return wide ? (h >= 5 && h <= 11) : (h >= 6 && h <= 10); }
static bool head_e(const Battle *b, int s, int wide) { int h = HEAD(b, s); return wide ? (h >= 1 && h <= 7) : (h >= 2 && h <= 6); }
static bool head_w(const Battle *b, int s, int wide) { int h = HEAD(b, s); return wide ? (h >= 9 && h <= 15) : (h >= 10 && h <= 14); }

/* rt_5199(k, s): your side's values are bytes in SO6 (+2D3), the enemy's the opponent's (SO2) */
int rt_skill(Battle *b, int k, int s)
{
    if (OWN(b, s) == -1) return msb(b, b->tbl_skill + k);
    return b->p->opp[k];
}

int ovl6_eff_speed(Battle *b, int s)   /* ovl6_f0a1: frigates sail a step faster */
{
    int v = b->speed[s];
    if (v != 0 && b->type[s] == 2) v++;
    return v;
}

int ovl6_xoff(Battle *b, int s)   /* ovl6_f939: where the picture is drawn for the heading */
{
    int h = b->turning[s] ? b->prev_heading[s] : b->heading[s];
    h <<= 1;
    if (b->turning[s]) h++;
    return msb(b, (b->type[s] == 2 ? 0x17C5 : 0x17A5) + h);
}

static int wind_penalty(Battle *b, int s)   /* ovl6_f39e: sailing into the wind */
{
    if (!b->p->wind_on || b->demo) return 0;
    int h = b->heading[s];
    switch (b->p->wind) {
    case 0: h += 8; break;
    case 1: h += 4; break;
    case 3: h -= 4; break;
    default: break;
    }
    if (h < 0) h += 16;
    if (h > 15) h -= 16;
    if (h > 4 && h < 12) return msb(b, 0x178E + h - 5);
    return 0;
}

static int time_bonus(Battle *b)   /* ovl6_f433: at the fastest time every ship sails as if faster */
{
    if (b->time != 0) return 0;
    int best = 0;
    for (int i = 0; i < BS; i++)
        if (b->x[i] != NOSHIP && b->speed[i] - wind_penalty(b, i) > best) best = b->speed[i];
    best = 8 - best;
    return best < 0 ? 0 : best;
}

static int heading_gap(Battle *b, int a, int c)   /* ovl6_fa90 */
{
    int h1 = b->heading[a], h2 = b->heading[c];
    if (h1 >= 8) h1 -= 8;
    if (h2 >= 8) h2 -= 8;
    int d = h1 - h2;
    if (d < 0) d = -d;
    if (d > 4) d = 8 - d;
    return d;
}

static bool held_by(Battle *b, int s)   /* ovl6_faf8: some ship is grappled to s */
{
    for (int i = 0; i < BS; i++) if (b->grappled[i] == s) return true;
    return false;
}

int rt_distance(Battle *b, int a, int c)   /* rt_4926 */
{
    int dx = X(b, a) - X(b, c), dy = Y(b, a) - Y(b, c);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx > dy ? (dy >> 2) + dx : (dx >> 2) + dy;
}

bool rt_struck_left(Battle *b)   /* rt_44b2: a ship that struck her colours (hull 6-7) */
{
    for (int i = 0; i < BS; i++)
        if (b->x[i] != NOSHIP && (b->hull[i] == 6 || b->hull[i] == 7)) return true;
    return false;
}

/* ------------------------------------------------------------------ smoke and splashes */

void ovl6_particles(Battle *b, int s)   /* ovl6_f4b5: the smoke of ship s drifts on */
{
    for (int i = 0; i < BPART; i++) {
        if (b->pstate[i] == 0 || b->powner[i] != s) continue;
        if (b->moved[s] == 0) b->moved[s] = 999;
        b->pnext[i] = (b->pnext[i] + 1) & 0xFF;
        if (b->pnext[i] > 103 ? b_random(b) < 40 || b->pnext[i] > 105 : false)
            b->pnext[i] = (b->pnext[i] & 0xFF) - 100;
    }
}

static void draw_particles(Battle *b, int s)   /* ovl6_f551 (close view) */
{
    for (int set = 0; set < BPART; set += 6) {
        if (b->pstate[set] == 0 || b->powner[set] != s) continue;
        int style = b->owner[s] == -1 ? 0 : -1;
        for (int k = 0; k < 6; k++) {
            int i = set + k, spr = 0;
            switch (b->pstate[i] & 0xFF) {
            case 2: spr = 0x4774; break;
            case 3: spr = 0x478E; break;
            case 4: spr = b->pvar[i] ? 0x47BC : 0x47A8; break;
            case 5: spr = b->pvar[i] ? 0x47E4 : 0x47D0; break;
            case 6: spr = b->pvar[i] ? 0x480C : 0x47F8; break;
            default: break;
            }
            if (spr) g_ship(b, BMEM_SO8 + spr, b->py[i] - b->vy, style, b->px[i] - b->vx, 0, 0, 1, 0);
        }
    }
}

/* ovl6_e4ad(spr, s, y, x): the sea over ship s (and its pennant in the overview),
 * and the smoke over it moved on a step */
static void erase_ship(Battle *b, int spr, int s, int y, int x)
{
    g_erase(b, spr, y, x, b->hull[s]);
    if (b->view == 2 && b->type[s] == 3) g_erase(b, 0x5AF, y - 2, x, 0);
    for (int set = 0; set < BPART; set += 6) {
        if (b->pstate[set] == 0 || b->powner[set] != s) continue;
        for (int k = 0; k < 6; k++) {
            int i = k + set, st = b->pstate[i] & 0xFF, sp = 0;
            if (st > 1 && st < 7) sp = BMEM_SO8 + mw(b, 0x182F + 2 * (st - 2));
            if (sp && b->view == 3) g_erase(b, sp, b->py[i] - b->vy, b->px[i] - b->vx, 0);
            if ((b->pnext[i] & 0xFF) == st) continue;
            b->pstate[i] = b->pnext[i] & 0xFF;
            if (b->pstate[i] > 6 && b->pstate[i] < 100) b->pnext[i] = b->pstate[i] = 0;
            b->py[i] -= 2;
            b->pvar[i] = b_random(b) < 64;
        }
    }
}

/* ------------------------------------------------------------------ the ships' orders */

void ovl6_mimic(Battle *b, int s)   /* ovl6_f6f9: ships told to mimic s set the same sails */
{
    static int depth = 0;
    if (depth > 8) return;
    depth++;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->order[i] != 2) continue;
        if (b->target[i] == -1) {
            b->order[i] = 4;
            if (i == b->sel) ovl6_order_text(b);
            continue;
        }
        if (b->target[i] != s) continue;
        b->sail_set[i] = b->sail_set[s];
        if (b->type[s] != 2 && b->type[i] == 2 && b->sail_set[s] > 1) b->sail_set[i]--;
        if (b->type[s] == 2 && b->type[i] != 2 && b->sail_set[s] < 6) b->sail_set[i]++;
        ovl6_mimic(b, i);
    }
    depth--;
}

void ovl6_turn_followers(Battle *b, int s)   /* ovl6_f80b: ships mimicking s turn as it turns */
{
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->order[i] != 2 || b->target[i] != s) continue;
        b->turn[i] = b->turn[s] > 0 ? 1 : -1;
        b->turn_wait[i] = 0;
    }
}

/* rt_433e(s, kind): the nearest enemy for s to chase (0), another (1), or the first
 * ship that struck her colours to board (2) */
int rt_nearest_target(Battle *b, int s, int kind)
{
    int best = 999, found = -1, mine = b->hull[s] + b->sails[s], side = b->owner[s];
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP) continue;
        if (side == -1 && b->owner[i] == -1 && b->hull[i] < 6) continue;
        if (side == 0 && b->owner[i] == 0 && mine > -5) continue;
        if (b->hull[i] > 5 && kind == 2) { found = i; break; }
        int d = rt_distance(b, s, i);
        if (b->hull[i] > 5 && kind == 0) d /= 4;
        if (b->type[i] == 3 && kind == 0) d /= 2;
        if (d >= best) continue;
        if (found != -1 && b_random(b) < 60 && kind == 0) continue;
        best = d;
        found = i;
    }
    b->target[s] = found;
    return found;
}

/* rt_49ab(s): one of s's own side ahead of it, within 100: the ship to follow */
bool rt_can_aim(Battle *b, int s)
{
    int best = 999, who = 0;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->owner[i] != b->owner[s] || i == s || b->sails[i] > 5) continue;
        int sy = 1, sx = 1;
        int dx = b->x[s] - b->x[i]; if (dx < 0) { dx = -dx; sx = -1; }
        int dy = b->y[s] - b->y[i]; if (dy < 0) { dy = -dy; sy = -1; }
        if (dy + dx >= best) continue;
        bool ahead = (head_n(b, s, 1) && sy >= 0 && dy > 4) ||
                     (head_s(b, s, 1) && sy <= 0 && dy > 4) ||
                     (head_e(b, s, 1) && sx <= 0 && dx > 2) ||
                     (head_w(b, s, 1) && sx >= 0 && dx > 2);
        if (!ahead) continue;
        best = dy + dx;
        who = i;
    }
    if (best < 100) { b->target[s] = who; return true; }
    return false;
}

static bool behind(Battle *b, int a, int c)   /* rt_4b35: c is astern of a */
{
    int dx = X(b, a) - X(b, c), dy = Y(b, a) - Y(b, c);
    return (head_n(b, a, 0) && dy < -10) || (head_s(b, a, 0) && dy > 10) ||
           (head_e(b, a, 0) && dx > 10) || (head_w(b, a, 0) && dx < -10);
}

static bool head_on(Battle *b, int a, int c)   /* rt_4ca0 */
{
    int dx = X(b, a) - X(b, c), dy = Y(b, a) - Y(b, c);
    return (head_n(b, a, 0) && head_s(b, c, 0) && dy > 0) || (head_s(b, a, 0) && head_n(b, c, 0) && dy < 0) ||
           (head_e(b, a, 0) && head_w(b, c, 0) && dx < 0) || (head_w(b, a, 0) && head_e(b, c, 0) && dx > 0);
}

/* rt_462c(a, c, keep): which way a should turn: toward c (keep 0, table SO6+253) or
 * to keep station on it (keep 1, SO6+293) */
static int turn_toward(Battle *b, int a, int c, int keep)
{
    int sy = 1, sx = 1;
    int dx = X(b, a) - X(b, c); if (dx < 0) { dx = -dx; sx = -1; }
    int dy = Y(b, a) - Y(b, c); if (dy < 0) { dy = -dy; sy = -1; }
    if (keep) {
        int lim = TYPE(b, a) == 2 ? 5 : 7;
        if (dx < lim) sx = -sx;
        if (dy < lim) sy = -sy;
        if (dx < 6) sx = 0;
        if (dy < 6) sy = 0;
    }
    bool n = head_n(b, a, 0), so = head_s(b, a, 0), e = head_e(b, a, 0), w = head_w(b, a, 0);
    int k = 0;
    if (n) { if (w) k = 32; if (e) k = 40; }
    else if (so) { k = 8; if (w) k += 40; if (e) k += 48; }
    else if (e) k = 16;
    else if (w) k = 24;
    if (sy < 0) { if (sx > 0) k += 2; else if (sx == 0) k++; }
    else if (sy > 0) { k += 3; if (sx > 0) k += 2; else if (sx == 0) k++; }
    else k += sx > 0 ? 6 : 7;
    if (a >= 0 && a < BS) b->turn_wait[a] = 3;
    if (keep) return msb(b, b->tbl_turn2 + k);
    bool cn = head_n(b, c, 0), cs = head_s(b, c, 0), ce = head_e(b, c, 0), cw = head_w(b, c, 0);
    if (k == 4 && head_w(b, a, 1)) k--;
    if (k == 9 && head_e(b, a, 1)) k++;
    if (k == 23 && head_s(b, a, 1)) k++;
    if (k == 30 && head_n(b, a, 1)) k = 33;
    if (k == 37 && (cs || cw)) k++;
    if (k == 43 && (cn || cw)) k++;
    if (k == 50 && (ce || cs)) k--;
    if (k == 56 && (cn || ce)) k--;
    return msb(b, b->tbl_turn + k);
}

static void keep_station(Battle *b, int s)   /* rt_4503 */
{
    int t = b->target[s];
    bool follow = b->order[s] == 3, close = false;
    int hs = 0, ht = 0;
    if (follow) {
        hs = b->heading[s];
        ht = HEAD(b, t);
        int gap = (t >= 0 && t < BS) ? heading_gap(b, s, t) : 0;
        int d = hs - ht; if (d < 0) d = -d;
        if (d > 3 && d < 13) gap += 8;
        if (gap <= 1) close = true;
    }
    int turn = turn_toward(b, s, t, 1);
    if (turn == 0 && follow) {
        if (hs < ht) turn = 1;
        if (hs > ht) turn = -1;
    }
    bool way = (turn > 0 && hs > ht) || (turn < 0 && hs >= ht);
    if (!way && close) return;
    b->turn[s] = turn;
    b->turn_wait[s] = 3;
}

static void tactics(Battle *b, int s)   /* rt_3c26: turn toward the nearest danger */
{
    int o = b->order[s];
    if (o == 2 || o == 3) return;
    int best = 999, who = -1;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || i == s) continue;
        if ((b->order[i] == 2 || b->order[i] == 3) && b->target[i] == s && b->owner[i] == b->owner[s]) continue;
        if (o == 6 && b->target[s] == i) continue;
        int d = rt_distance(b, s, i);
        if (b->grappled[s] == i && d > 25) b->grappled[s] = -1;
        if (head_on(b, s, i)) d /= 2;
        if (d < best) { best = d; who = i; }
    }
    if (HULL(b, who) > b->hull[s] + 2 && TYPE(b, who) == 2) best <<= 1;
    if (rt_skill(b, 17, s) && OWN(b, who) == -1) best <<= 1;
    if (rt_skill(b, 15, s) > best || (best < 25 && OWN(b, who) == b->owner[s])) {
        b->turn[s] = turn_toward(b, s, who, 0);
        b->turn_wait[s] = 2;
    }
}

static void steer(Battle *b, int s)   /* rt_3580 */
{
    int h = b->heading[s], t = b->target[s], o = b->order[s];
    int *turn = &b->turn[s];
    if ((o == 4 || o == 6 || (o == 5 && b->hull[s] < 6)) && b->sail_set[s] < 6 && (s != b->sel || b->demo))
        b->sail_set[s]++;
    if (o != 3) {
        int x = b->x[s], y = b->y[s], lim = 0, sh = b->shore;
        if (h >= 7 || h < 2) {
            if (x < 35 || (sh == 2 && x < 234) || (sh == 7 && y - 5 > x) || (sh == 8 && 0x190 - y > x)) {
                lim = sh == 7 ? 10 : sh == 8 ? 14 : 12;
                *turn = (h < lim && h > 6) ? -2 : 2;
            }
        }
        if (!(h > 9 && h <= 14) && lim == 0) {
            if (x > 0x1A9 || (sh == 1 && x > 240) || (sh == 5 && y + 100 < x) || (sh == 6 && 0x208 - y < x)) {
                lim = sh == 5 ? 2 : sh == 6 ? 6 : 4;
                *turn = (h > lim && h < 10) ? 2 : -2;
            }
        }
        if ((h >= 12 || h <= 4) && lim == 0 && (y < 45 || (sh == 3 && y < 200)))
            *turn = h > 4 ? -2 : 2;
        if (h >= 4 && h <= 12 && lim == 0 && (y > 0x17E || (sh == 4 && y > 0x100)))
            *turn = h < 8 ? -2 : 2;
    } else if (t == -1) {
        b->order[s] = b->practice ? 1 : 4;
        if (s == b->sel) ovl6_order_text(b);
    } else if (!(s == b->sel && !b->demo)) {
        int d = rt_distance(b, s, t);
        int faster = ovl6_eff_speed(b, s) - ovl6_eff_speed(b, t);
        if (faster > 0) {
            if (d < 30) b->sail_set[s] = 0;
            else if (d < 35) b->sail_set[s]--;
            b->speed[s] = b->sail_set[s] + 3;
            if (d < 40 && ovl6_eff_speed(b, s) > ovl6_eff_speed(b, t) + 1) b->sail_set[s]--;
        } else if (faster == 0) {
            if (d < 30) b->sail_set[s]--;
            else if (d > 40) b->sail_set[s]++;
        } else if (d > 30) {
            b->sail_set[s]++;
            if (b->sail_set[s] < 4) b->sail_set[s] = 4;
        }
        if (b->sail_set[s] < 0) b->sail_set[s] = 0;
        if (b->sail_set[s] > 6) b->sail_set[s] = 6;
        b->speed[s] = b->sail_set[s] + 3;
        b->speed_wait[s] = 0;
        if (b->sail_set[s] == 0) b->speed[s] = 0;
    }
    tactics(b, s);
    if (*turn == 0) {
        if (o == 3) keep_station(b, s);
        else if (o == 4 || o == 6) {
            if (o == 4 && b_random(b) < rt_skill(b, 16, s)) rt_nearest_target(b, s, 1);
            if (b_random(b) < 6 && behind(b, s, t) && o == 4) {
                if (b->p->rnd < 3) *turn += 4; else *turn -= 4;
            } else {
                bool give_up = false;
                if (t == -1) {
                    if (o == 6) {
                        if (rt_struck_left(b)) rt_nearest_target(b, s, 2);
                        else give_up = true;
                    } else rt_nearest_target(b, s, 0);
                }
                if (!give_up && o == 6 && !rt_struck_left(b)) give_up = true;
                if (give_up) {
                    b->order[s] = b->practice ? 1 : 4;
                    ovl6_order_text(b);
                }
                keep_station(b, s);
            }
        } else if (o == 5 && b_random(b) < 15) {
            rt_nearest_target(b, s, 1);
            *turn = turn_toward(b, s, b->target[s], 0);
            if (*turn) b->turn_wait[s] = 3;
        }
    }
    if (*turn == 0 && b->p->wind_on && b->order[s] != 2) {   /* not head to wind */
        int w = b->heading[s];
        switch (b->p->wind) {
        case 0: w += 8; break;
        case 1: w += 4; break;
        case 3: w -= 4; break;
        default: break;
        }
        w = wrap16(w);
        if (w == 8) {
            *turn = b_random(b) < 64 ? 1 : -1;
            b->turn_wait[s] = 3;
        }
    }
    if (b->sails[s] > 5 || b->sail_set[s] == 0) *turn = 0;
    ovl6_mimic(b, s);
}

static void crew_choice(Battle *b, int s)   /* rt_3e0d: the crew goes to load the other guns */
{
    if (s == b->sel && !b->demo) return;
    if (b->crew_side[s] == 0 && b->gun_l[s] == 13 && b->gun_r[s] != 13) b->crew_side[s] = 1;
    if (b->crew_side[s] == 1 && b->gun_r[s] == 13 && b->gun_l[s] != 13) b->crew_side[s] = 0;
}

void rt_ai_step(Battle *b)   /* rt_34bf: one ship a step thinks */
{
    if (++b->ai_ship >= 6) b->ai_ship = 0;
    int s = b->ai_ship;
    if (b->x[s] == NOSHIP) return;
    if (b->owner[s] != 0 && b->order[s] <= 1) return;
    int t = b->target[s];
    if (X(b, t) == NOSHIP || ((b->order[s] == 2 || b->order[s] == 3) && HULL(b, t) > 5)) b->target[s] = -1;
    crew_choice(b, s);
    steer(b, s);
}

/* rt_41bc(s, x, y): is an enemy of s at (x, y)? */
static bool enemy_at(Battle *b, int s, int x, int y)
{
    if (b->practice) return false;
    int ry = 4, rx = 4;
    x -= 2; y -= 2;
    if (head_w(b, s, 0) || head_e(b, s, 0)) { ry = 2; y++; }
    else { rx = 2; x++; }
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->hull[i] > 7 || i == s) continue;
        if (b->hull[i] > 5 && s != b->sel) continue;
        if (b->owner[i] == b->owner[s]) continue;
        int h = b->heading[i], w, d;
        if (b->type[i] == 2) { w = mw(b, 0x1000 + 2 * h); d = mw(b, 0x1020 + 2 * h); }
        else { w = mw(b, 0x1040 + 2 * h); d = mw(b, 0x1060 + 2 * h); }
        if (b->x[i] - rx > x || w + b->x[i] <= x || b->y[i] - ry >= y || d + b->y[i] <= y) continue;
        return true;
    }
    return false;
}

/* rt_3fa4(s, side): the elevation (0-rt_skill 1) at which side's guns would hit an enemy */
int rt_aim(Battle *b, int s, int side)
{
    int h = wrap16(b->heading[s] + (side ? 4 : -4));
    int turning = b->turning[s];
    int a = turning ? msb(b, 0xF76 + h) : F42(b, h);
    int c = turning ? msb(b, 0xF86 + h) : F52(b, h);
    if (c == 60) c = 0;
    if (a == 60) a = 0;
    int sy = mw(b, (turning ? 0xFA0 : 0xF6C) + 2 * c);
    if (h < 4 || h > 12) sy = -sy;
    int sx = mw(b, (turning ? 0xF96 : 0xF62) + 2 * a);
    if (h > 8) sx = -sx;
    int hd = b->heading[s], x = b->x[s], y = b->y[s];
    if (b->type[s] == 2) { x += msb(b, 0xFC0 + hd); y += msb(b, 0xFE0 + hd); }
    else { x += msb(b, 0xFD0 + hd); y += msb(b, 0xFF0 + hd); }
    sx *= 2; sy *= 2;
    x += sx; y += sy;
    for (int e = 0; rt_skill(b, 1, s) >= e; e++) {
        if (enemy_at(b, s, x, y)) {
            if (side) b->aim_r_set[s] = e; else b->aim_l_set[s] = e;
            return 1;
        }
        x += sx; y += sy;
    }
    return 0;
}

int rt_enemy_fire(Battle *b)   /* rt_3eaa: ships whose guns bear fire (yours too, but the one in the panel) */
{
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->hull[i] > 5) continue;
        if (b_random(b) > rt_skill(b, 14, i)) continue;
        if (i == b->sel && !b->demo) continue;
        if (rt_aim(b, i, 0) && b->gun_l[i] == 13) {
            b->crew_side[i] = 0;
            b->enemy_fired = true;
            if (ovl5_fire(b, i)) return 1;
        }
        if (rt_aim(b, i, 1) && b->gun_r[i] == 13) {
            b->enemy_fired = true;
            b->crew_side[i] = 1;
            if (ovl5_fire(b, i)) return 1;
        }
    }
    return 0;
}

int rt_closest_own(Battle *b)   /* rt_58bc (the demo): your ship nearest an enemy */
{
    int best = 999, who = 0;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->owner[i] != -1 || b->hull[i] > 5) continue;
        for (int j = 0; j < BS; j++) {
            if (b->x[j] == NOSHIP || b->owner[j] != 0 || b->hull[j] > 5) continue;
            int d = rt_distance(b, j, i);
            if (d < best) { best = d; who = i; }
        }
    }
    return who;
}

bool rt_enemy_near(Battle *b)   /* rt_597e (the demo) */
{
    int best = 999;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->owner[i] != 0) continue;
        int d = rt_distance(b, b->sel, i);
        if (d < best) best = d;
    }
    return best < 50;
}

void rt_sort_ships(Battle *b)   /* rt_5290: one pass of sorting the drawing order by y */
{
    for (int k = 0, j = 1; j < 6; k++, j++) {
        int a = b->draw_order[k], c = b->draw_order[j];
        if (b->x[a] == NOSHIP || b->x[c] == NOSHIP) continue;
        int ya = b->y[a], yc = b->y[c];
        if (b->type[a] == 2) ya -= 4;
        if (b->type[c] == 2) yc -= 4;
        if (ya > yc) { b->draw_order[k] = c; b->draw_order[j] = a; }
    }
}

static void off_the_map(Battle *b, int s)   /* rt_545e: a ship sailed out of the battle */
{
    if (b->x[s] >= -42 && b->x[s] <= 0x20A && b->y[s] >= -52 && b->y[s] <= 0x210) return;
    if (!b->practice) ovl6_message(b, 125, b->name[s], SO6 + 0x23C, 0);   /* The X retreated. */
    b->order[s] = 888;
    b->x[s] = NOSHIP;
    if (b->sel == s && !rt_battle_over(b)) {
        rt_next_ship(b);
        rt_flag_tune(b);
    }
}

void rt_draw_all(Battle *b)   /* rt_5367: ships that left the battle */
{
    for (int i = 0; i < BS; i++)
        if (b->x[i] != NOSHIP) off_the_map(b, i);
}

/* ------------------------------------------------------------------ speeds */

static void speed_steps(Battle *b, int s)   /* ovl6_efbb: a faster speed takes effect at once */
{
    int bonus = time_bonus(b);
    int sx = F42(b, b->heading[s]) * FAA(b, ovl6_eff_speed(b, s) + bonus);
    int sy = F52(b, b->heading[s]) * FAA(b, ovl6_eff_speed(b, s) + bonus);
    if (b->xcount[s] <= sx || b->ycount[s] <= sy) return;
    b->xcount[s] = sx;
    b->ycount[s] = sy;
    if (b->xcount[s] > 100) b->xcount[s] = 100;
    if (b->ycount[s] > 100) b->ycount[s] = 100;
}

void ovl6_speeds(Battle *b)   /* ovl6_f0d2: each ship's speed toward its sails, the wind and damage */
{
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP) continue;
        int *sp = &b->speed[i];
        int want = b->sail_set[i] + 3;
        if (b->sail_set[i] == 0) want = 0;
        int into = 0, pen = wind_penalty(b, i);
        if (*sp <= want) into = pen;
        if (b->order[i] == 3 && i != b->sel) pen /= 2;
        if (b->sail_set[i] != 0 && pen == 10 && b->order[i] != 1) pen = 3;
        want -= pen;
        int torn = b->sails[i] / 2;
        if (b->sails[i] > 4) torn += b->sails[i] - 4;
        torn -= 6 - b->sail_set[i];
        if (torn < 0) torn = 0;
        if (want - 1 > torn) want -= torn;
        if (want < 0) want = 0;
        if (b->sails[i] > 5) want = 0;
        if (want == 1 && into == 0 && b->sail_set[i] > 5) want = 2;
        if (*sp == want) continue;
        if (--b->speed_wait[i] >= 0) {
            if (want != 0) continue;
            if (*sp >= want) continue;
        }
        if (b->order[i] == 3 && i != b->sel) b->speed_wait[i] = 0;
        else b->speed_wait[i] = b->type[i] == 2 ? 1 : 2;
        if (*sp > want) {
            (*sp)--;
            if (into) {
                b->speed_wait[i] <<= 2;
                if (b->view == 2) b->speed_wait[i] <<= 1;
            }
        } else if (*sp < want) {
            bool hold = *sp <= 1 && b_random(b) >= 32 && (b->order[i] == 1 || b->order[i] == 4) && !b->enemy_fired;
            if (!hold) (*sp)++;
        }
        if (i == b->sel) g_speed_bar(b, *sp);
        speed_steps(b, i);
    }
}

/* ------------------------------------------------------------------ drawing and turning */

static int pick_sprite(Battle *b, int s)
{
    if (b->turning[s] && b->view == 3) {
        int h = b->prev_heading[s];
        return b->type[s] == 2 ? b->spr_frigate_turn[h] : b->spr_ship_turn[h];
    }
    if (b->type[s] == 2 || b->view != 3) return b->heading_spr[b->heading[s]];
    return b->spr_ship[b->heading[s]];
}

static void ship_pos(Battle *b, int s, int *x, int *y)
{
    if (b->view == 3) { *x = ovl6_xoff(b, s) + (b->x[s] - b->vx); *y = b->y[s] - b->vy; }
    else { *x = b->x[s] / 6; *y = b->y[s] / 6; }
}

static void turn_step(Battle *b, int s)   /* rt_59ec: half a point of the turn at a time in the close view */
{
    if (b->turn[s] == 0 || b->sails[s] >= 6) { b->turning[s] = 0; return; }
    if (b->view == 2) b->turn_wait[s]--;
    if (--b->turn_wait[s] >= 0) return;
    ovl6_turn_followers(b, s);
    if (b->order[s] == 2) b->turn_wait[s] = 10;
    else if (b->type[s] != 2 && SAILS(b, b->target[s]) < 6) b->turn_wait[s] = 3;
    else b->turn_wait[s] = 2;
    int x, y;
    ship_pos(b, s, &x, &y);
    erase_ship(b, pick_sprite(b, s), s, y, x);
    if (b->turn[s] > 0) {
        if (b->view == 2 || b->turning[s]) { b->turn[s]--; b->heading[s]++; b->turning[s] = 0; }
        else b->turning[s] = 1;
    } else {
        if (b->view == 2 || b->turning[s]) { b->turn[s]++; b->heading[s]--; b->turning[s] = 0; }
        else b->turning[s] = 1;
    }
    if (b->heading[s] < 0) b->heading[s] = 15;
    if (b->heading[s] > 15) b->heading[s] = 0;
    b->xcount[s] = F42(b, b->heading[s]) * FAA(b, ovl6_eff_speed(b, s));
    b->ycount[s] = F52(b, b->heading[s]) * FAA(b, ovl6_eff_speed(b, s));
    b->half[s] = 0;
    if (s == b->sel) ovl6_compass(b);
}

static void sinking_sound(Battle *b)
{
    for (int k = 0; k < 3; k++)
        for (int v = 30; v <= 110; v++) {
            if (b_random(b) < 115) v--;
            if (b_random(b) < v) b_click(b);
            b_delay(b, 2);
        }
}

/* ovl5_afc6: every ship drawn, in order of y: erased where it was if it moved,
 * turned a step, drawn; a ship sunk out of sight is gone, with its message */
void ovl5_draw_ships(Battle *b)
{
    for (int k = 0; k < BS; k++) {
        int s = b->draw_order[k];
        if (b->x[s] == NOSHIP) continue;
        int hull = b->hull[s], spr = pick_sprite(b, s), x, y;
        int mv = b->moved[s];
        ship_pos(b, s, &x, &y);
        if (mv != 0) {
            int dy = 0;
            if (mv == 999) { mv = 0; b->moved[s] = 0; }
            if (mv > 78) { mv -= 80; dy = -1; }
            if (mv < -78) { mv += 80; dy = 1; }
            erase_ship(b, spr, s, dy + y, x - mv);
            if (hull >= 27) {
                b->x[s] = NOSHIP;
                if (b->sunk_how != 0) {
                    sinking_sound(b);
                    int n = b->name[b->sunk_ship];
                    if (b->sunk_how == 1) ovl6_message(b, n, SO6 + 0x20B, 0, -1);        /* sinks under heavy fire */
                    else if (b->sunk_how == 3) ovl6_message(b, n, 0x31E, 0, -1);          /* hits a reef and sinks */
                    else if (b->sunk_how == 10) ovl6_message(b, 0x335, n, 0, -1);         /* You sank the */
                    else ovl6_message(b, n, SO6 + 0x223, 0, -1);                          /* sinks after a collision */
                    for (int i = 0; i < BPART; i++) if (b->powner[i] == s) b->pstate[i] = 0;
                    b->sunk_how = 0;
                    b->flag_tune = false;
                }
                if (b->sel == b->sunk_ship && !rt_battle_over(b)) {
                    rt_next_ship(b);
                    rt_flag_tune(b);
                    b_getkey(b);
                    b_getkey(b);
                }
                continue;
            }
            b->moved[s] = 0;
            turn_step(b, s);
            if (b->turning[s] && b->view == 3) {
                int h = b->heading[s];
                if (b->turn[s] < 0 && --h < 0) h = 15;
                b->prev_heading[s] = h;
                spr = b->type[s] == 2 ? b->spr_frigate_turn[h] : b->spr_ship_turn[h];
            } else spr = (b->type[s] == 2 || b->view != 3) ? b->heading_spr[b->heading[s]] : b->spr_ship[b->heading[s]];
            if (b->view == 3) ship_pos(b, s, &x, &y);
        }
        int sails = b->sails[s];
        if (b->view == 2) sails = b->sails[s] > 5 ? 4 : 0;
        g_ship(b, spr, y, b->owner[s], x, b->half[s], sails, b->type[s], hull);
        if (b->view == 2 && b->type[s] == 3)
            g_ship(b, 0x5AF, y - 2, b->owner[s], x, b->half[s], sails, 3, 0);
        if (b->view == 3) draw_particles(b, s);
    }
    if (b->view == 2) g_box(b, b->bx, b->by, 1);
}

/* ------------------------------------------------------------------ collisions, grappling and running aground (ovl5_bc12) */

static void crash_sound(Battle *b)
{
    b_sound(b, 0x2710);
    b_cost(b, 10000 * 30.0);   /* 10000 times random(), a click 1 in 25 */
    for (int i = 0; i < 10000; i++) if (b_random(b) < 5) b_click(b);
    b_sound_off(b);
    b->p->zoomed = false;
}

static void box_of(Battle *b, int i, int *x, int *y, int *w, int *h)
{
    int hd = b->heading[i];
    if (b->type[i] == 2) {
        *x += mw(b, 0x168E + 2 * hd); *y += mw(b, 0x16CE + 2 * hd);
        *w = mw(b, 0x1000 + 2 * hd) + mw(b, 0x16AE + 2 * hd);
        *h = mw(b, 0x1020 + 2 * hd) + mw(b, 0x16EE + 2 * hd);
    } else {
        *x += mw(b, 0x170E + 2 * hd); *y += mw(b, 0x174E + 2 * hd);
        *w = mw(b, 0x1040 + 2 * hd) + mw(b, 0x172E + 2 * hd);
        *h = mw(b, 0x1060 + 2 * hd) + mw(b, 0x176E + 2 * hd);
    }
}

static void lost_flagship_check(Battle *b, int s)
{
    if (b->sel == s && !rt_battle_over(b)) {
        rt_next_ship(b);
        rt_flag_tune(b);
        b_getkey(b);
        b_getkey(b);
    }
}

static int collisions(Battle *b, int s)
{
    bool frig = b->type[s] == 2;
    int strength = b->hull[s];
    /* rt_5199(17) with no second argument: WAS.EXE takes the ship from a stale local (the
       last one this routine looked at) */
    if (b->owner[s] == 0 && rt_skill(b, 17, b->stale) != 0) strength -= 2;
    if (b->hull[s] > 7) return b->hull[s];
    int x0 = ovl6_xoff(b, s) + b->x[s], y0 = b->y[s] + 4;
    int ax = x0, ay = y0;
    if (b->shore) {
        if (b->view == 3) { ay += 10; ax += 4; }
        bool aground = false;
        switch (b->shore) {
        case 1: aground = ax > 0x186; break;
        case 2: aground = ax < 84; break;
        case 3: aground = ay < 102; break;
        case 4: aground = ay > 0x164; break;
        case 5: aground = ay + 0x102 < ax; break;
        case 6: aground = 0x29E - ay < ax; break;
        case 7: aground = ay - 148 > ax; break;
        case 8: aground = 254 - ay > ax; break;
        }
        b->stale = aground;
        if (aground) {
            b->hull[s] = frig ? 14 : 9;
            b->sunk_ship = s;
            b->sunk_how = 3;
            crash_sound(b);
            return 0;
        }
    }
    if (b->practice || b->hull[s] > 5) return b->hull[s];
    int bx = x0, by = y0, bw, bh;
    box_of(b, s, &bx, &by, &bw, &bh);
    for (int o = 0; o < BS; o++) {
        b->stale = o;
        if (b->x[o] == NOSHIP || b->hull[o] > 7 || o == s || b->grappled[s] == o) continue;
        bool ofrig = b->type[o] == 2;
        int st = b->hull[o];
        if (st < 6) {
            if (frig) st -= 4;
            if (ofrig) st += 4;
            if (b->owner[o] == 0 && rt_skill(b, 17, o) != 0) st -= 2;
            if (held_by(b, o)) st -= 8;
        }
        if (st < strength) continue;
        int ox = ovl6_xoff(b, o) + b->x[o] - 2, oy = b->y[o] + 2, ow, oh;
        box_of(b, o, &ox, &oy, &ow, &oh);
        ow += 3; oh += 2;
        bool side = false, friendly = false;
        if (b->hull[o] > 5 || heading_gap(b, o, s) < 3 || b->demo) {
            if (head_w(b, o, 0) || head_e(b, o, 0)) { oy -= 3; oh += 4; }
            else { ox -= 2; ow += 4; }
            side = true;
        }
        if (!(bh + by > oy && oh + oy > by && bw + bx > ox && ow + ox > bx)) continue;
        if (side) {
            /* alongside: grappling, boarding or aid */
            b->speed[s] = 2;
            b->speed[o] = 2;
            if (b->owner[o] == b->owner[s] && b->sel != o && b->order[o] != 6) b->order[o] = 4;
            bool aid = b->hull[o] >= 6 || b->owner[o] == b->owner[s];
            if (!aid) {
                ovl5_draw_ships(b);
                ovl5_draw_ships(b);
                ovl6_message(b, b->name[s], 0x343, b->name[o], 1);   /* X grapples Y */
                for (int f = 0x898; f > 0x3E8; f -= 200) { b_sound(b, f); b_delay(b, 0x320); }
                b_sound_off(b);
                g_clear_row(b, 24);
                g_print(b, 0x34E);                                  /* Zoom Pass */
                int was = b->menu;
                b->menu = 'B';
                int k;
                for (;;) {
                    k = b->demo ? 'z' : b_getkey(b);
                    if (k == 'p' || k == 'P') { b_beep(b); BLOGN("you Pass: no boarding"); break; }
                    if (k == 'z' || k == 'Z') { b_beep(b); BLOGN("you Zoom: to the boarding"); break; }
                }
                int winner = -1;
                if (k == 'z' || k == 'Z') {
                    winner = ovl7_boarding(b, o, s);
                    BLOGN("the boarding is over: %s%s", winner == -1 ? "you left it" : "won by ", winner == -1 ? "" : blog_ship(b, winner));
                    b->menu = 0;
                    was = 0;
                    if (head_w(b, o, 0) || head_e(b, o, 0)) b->y[o] = oy >= by ? by + 5 : by - 5;
                    else b->x[o] = ox >= bx ? bx + 5 : bx - 5;
                    g_clear_row(b, 4);
                    ovl6_background(b);
                    ovl5_draw_ships(b);
                    ovl6_panel(b);
                    if (b->menu == 0) ovl6_commands(b); else ovl6_change_prompt(b);
                    ovl6_menu_mark(b);
                    while (b_getkey(b)) ;
                    if (b->view == 2) g_box(b, b->bx, b->by, 1);
                    if (winner == o) {
                        b->owner[s] = b->owner[o];
                        b->order[s] = 4;
                        lost_flagship_check(b, s);
                        b->p->zoomed = true;
                        if (b->sel == o) ovl6_speed_icon(b);
                        b->target[o] = -1;
                        b->flag_tune = false;
                        for (int i = 0; i < BS; i++)
                            if (b->grappled[i] == s && b->owner[i] != b->owner[s]) b->grappled[i] = -1;
                        goto fast;
                    }
                }
                b->menu = was;
                g_clear_row(b, 23);
                ovl6_panel(b);
                if (b->menu == 0) ovl6_commands(b); else ovl6_change_prompt(b);
                ovl6_menu_mark(b);
            }
            if (b->owner[s] == b->owner[o] && b->hull[o] < b->hull[s] + 2 && o != b->sel && s != b->sel &&
                b->sails[o] < b->sails[s] + 2)
                continue;
            friendly = false;
            if (b->owner[s] == b->owner[o]) {
                ovl6_message(b, b->name[s], 0x35C, b->name[o], 1);   /* X aids the Y */
                friendly = true;
            } else if (b->owner[s] == 0) {
                if (b->owner[o] == -1 && rt_skill(b, 17, o) == 0) ovl6_message(b, b->name[o], SO6, 0, 0);   /* strikes her colors */
                else ovl6_message(b, b->name[s], SO6 + 21, b->name[o], 0);                                /* boards the */
            } else ovl6_message(b, b->name[s], SO6 + 21, b->name[o], 1);
            b->hull[o] = (b->hull[o] + b->hull[s]) / 2;
            if (b->hull[o] > 5) b->hull[o] = 5;
            b->sails[o] = (b->sails[o] + b->sails[s]) / 2;
            if (b->sails[o] > 5) b->sails[o] = 5;
            if (b->sel == o) ovl6_speed_icon(b);
            b->owner[o] = b->owner[s];
            if (!friendly) lost_flagship_check(b, o);
            b->p->zoomed = true;
            if (b->sel == s) ovl6_speed_icon(b);
            b->target[s] = -1;
            b->flag_tune = false;
            for (int i = 0; i < BS; i++)
                if (b->grappled[i] == o && b->owner[i] != b->owner[o]) b->grappled[i] = -1;
        fast:
            b->grappled[s] = o;
            b->grappled[o] = s;
            if (b->x[b->sel] == NOSHIP) return b->x[b->sel];
            for (int i = 0; i < BS; i++) if (b->target[i] == i) b->target[i] = -1;
            b->stale = 6;
            return 0;
        }
        /* a collision: the weaker sinks */
        if (b->grappled[s] == o) continue;
        int ds = b->hull[s] + 3, dd = b->hull[o] + 3;
        if (frig) ds <<= 1;
        if (ofrig) dd <<= 1;
        if (b_random(b) < 64) ds += 2; else dd += 2;
        int *loser, *other;
        bool lfrig, ofr;
        if (ds > dd) { loser = &b->hull[s]; other = &b->hull[o]; lfrig = frig; ofr = ofrig; }
        else { loser = &b->hull[o]; other = &b->hull[s]; lfrig = ofrig; ofr = frig; }
        *loser = lfrig ? 14 : 9;
        *other += 3;
        if (*other == 6 || *other == 7) *other = 5;
        if (ofr && *other > 7) *other = 14;
        if (b->hull[s] > 7) { b->sunk_ship = s; b->sunk_how = 2; }
        if (b->hull[o] > 7) { b->sunk_ship = o; b->sunk_how = 2; }
        crash_sound(b);
        return 0;
    }
    b->stale = 6;
    return 0;
}

/* ------------------------------------------------------------------ sailing */

static int drift_heading(Battle *b, int h)   /* ovl5_b4c5: an anchored ship drifts with the wind */
{
    (void)h;
    switch (b->p->wind) {
    case 0: return 8;
    case 1: return 12;
    case 3: return 4;
    default: return 0;
    }
}

static void sail_close(Battle *b, int check)   /* ovl5_b4c5 (the close view) */
{
    int bonus = time_bonus(b);
    for (int s = 0; s < BS; s++) {
        if (b->x[s] == NOSHIP) continue;
        int h = b->heading[s];
        if (check) collisions(b, s);
        if (b->hull[s] >= 8) {
            if (++b->hull[s] > 9) { b->y[s]++; b->moved[s] = 80; }
            continue;
        }
        int v = ovl6_eff_speed(b, s);
        if (b->speed[s] == 0 && b->p->wind_on && b->ending == 0) { h = drift_heading(b, h); v = 1; }
        if (v) v += bonus;
        if (--b->xcount[s] <= 0) {
            int m = F42(b, h);
            if (m == 1 && b_random(b) < 64) m = 2;
            b->xcount[s] = FAA(b, v) * m;
            if (b->xcount[s] > 100) b->xcount[s] = 100;
            if (v == 0) { b->moved[s] = 999; continue; }
            if (h > 0 && h < 8) {
                if (b->half[s]) { b->x[s]++; b->half[s] = 0; b->moved[s] = 1; }
                else { b->half[s] = 1; b->moved[s] = 999; }
            }
            if (h > 8) {
                if (!b->half[s]) { b->x[s]--; b->half[s] = 1; b->moved[s] = -1; }
                else { b->half[s] = 0; b->moved[s] = 999; }
            }
        }
        if (--b->ycount[s] > 0) continue;
        int m = F52(b, h);
        if (m == 1 && b_random(b) < 64) m = 2;
        b->ycount[s] = FAA(b, v) * m;
        if (b->ycount[s] > 100) b->ycount[s] = 100;
        if (v == 0) { b->moved[s] = 999; continue; }
        if (h > 12 || h < 4) {
            b->y[s]--;
            if (b->moved[s] == 999) b->moved[s] = 0;
            b->moved[s] -= 80;
        }
        if (h > 4 && h < 12) {
            b->y[s]++;
            if (b->moved[s] == 999) b->moved[s] = 0;
            b->moved[s] += 80;
        }
    }
    rt_draw_all(b);
}

static void sail_overview(Battle *b)   /* ovl5_b841: the overview, where a byte is 6 units */
{
    int bonus = time_bonus(b);
    for (int s = 0; s < BS; s++) {
        if (b->x[s] == NOSHIP) continue;
        int h = b->heading[s];
        collisions(b, s);
        if (b->hull[s] >= 8) { b->hull[s] = 27; b->moved[s] = 999; continue; }
        int v = ovl6_eff_speed(b, s);
        if (b->speed[s] == 0 && b->p->wind_on && b->ending == 0) { h = drift_heading(b, h); v = 1; }
        if (v) v += bonus;
        if (--b->xcount[s] <= 0) {
            b->xcount[s] = F42(b, h) * FAA(b, v);
            if (b->xcount[s] > 100) b->xcount[s] = 100;
            if (v == 0) { b->moved[s] = 999; continue; }
            if (h > 0 && h < 8) {
                if (b->sub[s]) {
                    b->x[s]++;
                    b->sub[s] = 0;
                    int r = (b->x[s] + 120) % 6;
                    if (r == 3) { b->half[s] = 1; b->moved[s] = 999; }
                    if (r == 0) { b->half[s] = 0; b->moved[s] = 1; }
                } else b->sub[s] = 1;
            }
            if (h > 8) {
                if (!b->sub[s]) {
                    b->x[s]--;
                    b->sub[s] = 1;
                    int r = (b->x[s] + 120) % 6;
                    if (r == 5) { b->half[s] = 1; b->moved[s] = -1; }
                    if (r == 2) { b->half[s] = 0; b->moved[s] = 999; }
                } else b->sub[s] = 0;
            }
        }
        if (--b->ycount[s] > 0) continue;
        b->ycount[s] = F52(b, h) * FAA(b, v);
        if (b->ycount[s] > 100) b->ycount[s] = 100;
        if (v == 0) { b->moved[s] = 999; continue; }
        if (h > 12 || h < 4) {
            b->y[s]--;
            if ((b->y[s] + 120) % 6 == 5) {
                if (b->moved[s] == 999) b->moved[s] = 0;
                b->moved[s] -= 80;
            }
        }
        if (h > 4 && h < 12) {
            b->y[s]++;
            if ((b->y[s] + 120) % 6 == 0) {
                if (b->moved[s] == 999) b->moved[s] = 0;
                b->moved[s] += 80;
            }
        }
    }
    rt_draw_all(b);
}

void ovl5_sail(Battle *b, int check)   /* ovl5_b4c5 */
{
    if (b->view == 2) { sail_overview(b); sail_overview(b); return; }
    sail_close(b, check);
}

/* ------------------------------------------------------------------ the guns */

static void tone_bursts(Battle *b, int gap, int n, int freq)   /* ovl5_ab6e */
{
    b_sound(b, freq);
    for (int k = 0; k < 5; k++)
        for (int d = n; d > 0; d--) { b_click(b); b_delay(b, d); b_click(b); b_delay(b, gap); }
}

static void explosion(Battle *b)   /* ovl5_a7ad: a ship blows up ([6CDC]) */
{
    int s = b->fired_sunk, h = b->turning[s] ? b->prev_heading[s] : b->heading[s];
    int y = b->y[s] - 1, x = ovl6_xoff(b, s) + (b->x[s] - 2);
    if (b->type[s] == 2) { y += msb(b, 0xFE0 + h); x += msb(b, 0xFC0 + h); }
    else { y += msb(b, 0xFF0 + h); x += msb(b, 0xFD0 + h); }
    x -= b->vx;
    y -= b->vy;
    bool close = b->view == 3;
    g_palette(b, 0);
    int dx = 1 - (b_random(b) & 3), dy;
    if (close) g_mask(b, b->boom_spr[0], y - 2, 0xAA, dx + x, 0);
    tone_bursts(b, 25, 45, 0xBB8);
    g_palette(b, 1);
    dx = 1 - (b_random(b) & 3);
    dy = b_random(b) & 3;
    if (close) g_mask(b, b->boom_spr[1], y - dy - 4, 0xAA, x - dx - 2, 0);
    tone_bursts(b, 17, 28, 0x7D0);
    g_palette(b, 0);
    dx = 4 - (b_random(b) & 7);
    dy = (b_random(b) & 3) << 1;
    if (close) g_mask(b, b->boom_spr[2], y - dy - 5, 0xAA, x - dx - 3, 0);
    tone_bursts(b, 12, 20, 0x514);
    g_palette(b, 1);
    dx = 3 - (b_random(b) & 7);
    dy = 6 - ((b_random(b) & 3) << 1);
    if (close) g_mask(b, b->boom_spr[3], y - dy - 7, 0xAA, x - dx - 4, 0);
    tone_bursts(b, 8, 14, 0x320);
    b_delay(b, 0x1F4);
    g_palette(b, 0);
    if (b->type[s] != 2) {
        dx = 2 - (b_random(b) & 7);
        dy = 7 - (b_random(b) & 3) * 3;
        if (close) g_mask(b, b->boom_spr[2], y - dy - 6, 0xAA, x - dx - 3, 0);
        tone_bursts(b, 17, 28, 0x7D0);
        b_delay(b, 0x1F4);
        g_palette(b, 1);
        dx = 1 - (b_random(b) & 7);
        dy = 7 - ((b_random(b) & 3) << 1);
        if (close) g_mask(b, b->boom_spr[3], y - dy - 7, 0xAA, x - dx - 4, 0);
        tone_bursts(b, 8, 14, 0x320);
        b_delay(b, 0x1F4);
        g_palette(b, 0);
    }
    b_sound(b, 0x1388);
    b_cost(b, 0x1F40 * 30.0);
    for (int i = 0; i < 0x1F40; i++) if (b_random(b) < 65) b_click(b);
    b_sound(b, 0x1B58);
    b_delay(b, 0x1F4);
    ovl6_background(b);
    g_palette(b, 1);
    ovl5_draw_ships(b);
    g_palette(b, 0);
    if (b->view == 2) g_box(b, b->bx, b->by, 1);
    b_delay(b, 0x1F4);
    b_sound_off(b);
    g_palette(b, 1);
}

/* ovl5_abb6(s, x, y, look): a shot of s's at (x, y): hits a ship there (look 1), or is
 * there one to hit (look 0)? */
static int shot_hits(Battle *b, int s, int x, int y, int look)
{
    if (b->practice) return 0;
    bool self = false;
    y++;
    int aim = b->crew_side[s] ? b->aim_r[s] : b->aim_l[s];
    int chance = aim != 0 ? 64 : 128;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->hull[i] > 7) continue;
        if (look == 0 && (b->owner[i] == b->owner[s] || b->hull[i] > 5)) continue;
        bool struck = b->hull[i] == 6 || b->hull[i] == 7;
        int left = ovl6_xoff(b, i) + (b->x[i] + 1), top = b->y[i] + 1, h = b->heading[i], w, d;
        if (b->type[i] == 2) { w = mw(b, 0x1000 + 2 * h); d = mw(b, 0x1020 + 2 * h); }
        else { w = mw(b, 0x1040 + 2 * h); d = mw(b, 0x1060 + 2 * h); }
        if (!(x >= left && w + left > x && y > top && d + top > y)) continue;
        if (i == s) { self = true; continue; }
        if (look == 0) return 1;
        if (b_random(b) < chance) {
            bool hit = b_random(b) < 32 || (b->type[i] == 1 && b_random(b) < 32) || b->type[i] == 2;
            if (hit) {
                if (b->sails[i] < 5 && (b_random(b) < aim * 30 || b->hull[i] > 4)) b->sails[i]++;
                else b->hull[i]++;
                if (b->hull[i] < 8 && b_random(b) < 1) { b->fired_sunk = i; b->hull[i] = 18; BLOGN("a shot blows up %s", blog_ship(b, i)); }
                else BLOGN("%s's shot hits %s", blog_str(b, b->name[s]), blog_ship(b, i));
                if (b->sails[i] > 7 || b->hull[i] > 5) b->sails[i] = 7;
                if (i == b->sel) ovl6_speed_icon(b);
            }
        }
        if (b->type[i] == 2 && b->hull[i] == 8) b->hull[i] = 14;
        if (b->hull[i] > 7) {
            b->sunk_ship = i;
            b->sunk_how = b->owner[i] == -1 ? 1 : 10;
            b->fired_struck = -1;
            b->p->zoomed = false;
        }
        if (b->hull[i] == 6 && !struck) {
            b->fired_struck = i;
            b->speed[i] = 0;
            b->sail_set[i] = 0;
            b->order[i] = 4;
            b->sails[i] = 7;
            b->p->zoomed = true;
        }
        if (b->view == 3) {
            int spr = BMEM_SO8 + (b_random(b) < 64 ? 0x482E : 0x4820);
            g_mask(b, spr, y - b->vy - 1, 0xAA, x - b->vx, 0);
        } else g_mask(b, SO6 + 0x309, (y - 1) / 6, 0xAA, x / 6, 0);
        if (b->moved[i] == 0) b->moved[i] = 999;
        b_sound(b, b_random(b) * 4 + 0xBB8);
        b_delay(b, 0x320);
        b_sound_off(b);
        return 1;
    }
    if ((b->view == 2 || self) && look) return 1;
    return 0;
}

static void splash(Battle *b, int spr, int y, int x, int half)
{
    g_ship(b, spr, y, -1, x, half, 0, 1, 0);
}

/* ovl5_936d(s): s fires the guns on the side its crew is at. Returns 1 if the
 * battle is to end (a key during the shot). */
int ovl5_fire(Battle *b, int s)
{
    int hd = b->turning[s] ? b->prev_heading[s] : b->heading[s];
    if ((b->crew_side[s] ? b->gun_r[s] : b->gun_l[s]) >= 13)
        BLOGV("%s fires a broadside to %s (elevation %d)", blog_ship(b, s), b->crew_side[s] ? "starboard" : "port",
              b->crew_side[s] ? b->aim_r_set[s] : b->aim_l_set[s]);
    int side = b->crew_side[s];
    bool big = b->type[s] != 2;
    int range;
    if (side == 0) {
        if (b->gun_l[s] < 13) return 0;
        if (b->gun_r[s] < 13 && b->demo) return 0;
        hd = wrap16(hd - 4);
        range = b->aim_l[s] = b->aim_l_set[s];
    } else {
        if (b->gun_r[s] < 13) return 0;
        hd = wrap16(hd + 4);
        range = b->aim_r[s] = b->aim_r_set[s];
    }
    range = range * 2 + 2;
    if (s == b->sel) {
        if (side || b->demo) {
            g_panel_sprite(b, PANEL + 0x716, b->crew_x + 47, 0, 1);
            g_panel_sprite(b, b->gun_r_spr[b->aim_r_set[s]], 125, 0, 0);
        } else {
            g_panel_sprite(b, PANEL + 0x6C4, b->crew_x + 42, 0, 1);
            g_panel_sprite(b, b->gun_l_spr[b->aim_l_set[s]], 90, 0, 0);
        }
        g_panel_copy(b, b->crew_scroll);
        b_delay(b, 1000);
        ovl5_draw_ships(b);
        ovl5_sail(b, 0);
        ovl5_draw_ships(b);
        ovl6_crew_panel(b);
        if (side || b->demo) {
            b->crew_x -= 3;
            g_panel_sprite(b, PANEL + 0x724, b->crew_x, 0, 0);
            g_panel_sprite(b, SO6 + 0x335, 213, 0, 0);
            g_panel_sprite(b, SO6 + 0x343, mw(b, b->tbl_pos_r + 2 * b->aim_r_set[s]), 0, 0);
        } else {
            b->crew_x += 3;
            g_panel_sprite(b, PANEL + 0x6D2, b->crew_x, 0, 0);
            g_panel_sprite(b, SO6 + 0x335, 181, 0, 0);
            g_panel_sprite(b, SO6 + 0x343, mw(b, b->tbl_pos_l + 2 * b->aim_l_set[s]), 0, 0);
        }
        g_panel_copy(b, b->crew_scroll);
    }
    /* the smoke of the broadside */
    int ph = b->heading[s];
    if (b->turning[s]) ph = msb(b, 0x1795 + b->prev_heading[s]);
    bool front = (side && (ph < 10 || ph == 15)) || (!side && (ph > 6 || ph < 2));
    if (b->view == 3) {
        int px = ovl6_xoff(b, s) + (b->x[s] - 1), py = b->y[s] - 1;
        if (side) { if (ph > 6) px -= 2; }
        else if (ph == 15 || ph == 0) px -= 2;
        int puffs = 6 - b->hull[s] / 2;
        if (big) {
            px += msb(b, 0xFD0 + ph); py += msb(b, 0xFF0 + ph);
            if (side) {
                if (ph < 2) px += 2;
                if (ph == 2 || ph == 7) px++;
                if (ph == 8) px--;
                if (ph == 9) py--;
                if (ph == 15) px += 6;
            } else {
                if (ph == 0) px--;
                if (ph == 1) px -= 2;
                if (ph == 8) px += 2;
                if (ph == 7) { px += 2; py--; }
                if (ph == 9 || ph == 13 || ph == 15) px++;
            }
        } else {
            px += msb(b, 0xFC0 + ph); py += msb(b, 0xFE0 + ph);
            if (side) {
                if (ph < 2) px++;
                if (ph == 9) px--;
                if (ph == 15) px += 3;
            } else {
                if (ph == 0) px--;
                if (ph == 1) px -= 2;
                if (ph == 8 || ph == 13 || ph == 15) px++;
            }
        }
        int sx, sy, alt = 0, tr = b->turning[s];
        if (hd == 0 || hd == 8 || (hd == 15 && tr) || (hd == 7 && tr)) { sx = 1; sy = 0; alt = 1; }
        else if ((hd >= 3 && hd <= 5) || (hd >= 11 && hd <= 13)) { sx = 0; sy = 2; }
        else if ((hd > 12 && hd < 15) || (hd > 4 && hd <= 7)) { sx = -1; sy = 1; }
        else { sx = 1; sy = 1; }
        if (hd >= 9 && hd <= 11) { py++; px--; }
        int oy = 0, ox = 0, n = 0, hf = 0, slot;
        for (slot = 0; slot < BPART && b->pstate[slot] != 0; slot += 6) ;
        bool flip = false;
        for (int k = 0; k < puffs; k++) {
            int spr = BMEM_SO8 + 0x4820, up = 0;
            if (b_random(b) < 64) { up = 1; spr = BMEM_SO8 + 0x482E; }
            if (front) {
                g_mask(b, spr, oy + py - up - b->vy, 0xAA, ox + px - b->vx, hf);
                g_mask(b, spr, py - oy - up - b->vy, 0xAA, px - ox - b->vx, hf);
            }
            if (slot < BPART) {
                b->pnext[slot] = b->pstate[slot] = front ? 1 : 101;
                b->powner[slot] = s;
                if (flip) { b->px[slot] = ox + px; b->py[slot] = oy + py - 2; }
                else { b->px[slot] = px - ox; b->py[slot] = py - oy - 2; }
                slot++;
                flip = !flip;
            }
            if (front) {
                b->shot_x[n] = ox + px - b->vx; b->shot_y[n] = oy + py - up - b->vy; n++;
                b->shot_x[n] = px - ox - b->vx; b->shot_y[n] = py - oy - up - b->vy; n++;
            }
            if (!alt) {
                if (hf) { hf = 0; ox += sx; oy += sy; }
                else hf = 1;
            } else { ox += sx; oy += sy; }
        }
    }
    /* the guns' roar */
    int step = (b->hull[s] << 4) + 70, top = big ? 5000 : 3500, f0 = 2000;
    if (b->owner[s] == 0) { f0 -= 1000; top -= 1000; }
    for (int f = f0; f < top; f += step) { b_sound(b, f); b_delay(b, 100); }
    b_sound_off(b);
    ovl5_sail(b, 0);
    ovl5_draw_ships(b);
    if (s == b->sel) {
        if (b->crew_side[b->sel] || b->demo) {
            b->gun_r[b->sel] = 0;
            ovl6_crew_panel(b);
            b->crew_x--;
            g_panel_sprite(b, PANEL + 0x724, b->crew_x, 0, 0);
        } else {
            b->gun_l[b->sel] = 0;
            ovl6_crew_panel(b);
            b->crew_x++;
            g_panel_sprite(b, PANEL + 0x6D2, b->crew_x, 0, 0);
        }
        g_panel_copy(b, b->crew_scroll);
        ovl6_fire_label(b);
    }
    if (b->view == 3) {
        for (int i = 0; i < BSHOTS; i++) {
            if (b->shot_y[i] != NOSHIP) g_erase(b, BMEM_SO8 + 0x4820, b->shot_y[i], b->shot_x[i], 0);
            b->shot_y[i] = NOSHIP;
        }
        for (int i = 0; i < BS; i++) ovl6_particles(b, i);
        ovl5_draw_ships(b);
    }
    /* where the shot falls */
    int tr = b->turning[s];
    int ca = tr ? msb(b, 0xF76 + hd) : F42(b, hd);
    int cb = tr ? msb(b, 0xF86 + hd) : F52(b, hd);
    if (cb == 60) cb = 0;
    if (ca == 60) ca = 0;
    int ty = mw(b, (tr ? 0xFA0 : 0xF6C) + 2 * cb);
    if (hd < 4 || hd >= 12) ty = -ty;
    int tx = mw(b, (tr ? 0xF96 : 0xF62) + 2 * ca);
    if (hd >= 8) tx = -tx;
    tx = tx * range + ovl6_xoff(b, s) + b->x[s];
    ty = ty * range + b->y[s];
    int h2 = b->heading[s];
    if (big) { tx += msb(b, 0xFD0 + h2); ty += msb(b, 0xFF0 + h2); }
    else { tx += msb(b, 0xFC0 + h2); ty += msb(b, 0xFE0 + h2); }
    int g = b_scnf_byte(b, 0x6867);
    if (b->practice) g = 10;
    if (side || b->demo) {
        b->gun_r[s] = g;
        if (b->gun_r[s] == 0) b->gun_r[s] = 3;
        if (b->demo) b->gun_r[s] = 10;
    } else {
        b->gun_l[s] = g;
        if (b->gun_l[s] == 0) b->gun_l[s] = 3;
    }
    int vx2, vy2;
    if (b->view == 2) { vx2 = tx / 6; vy2 = ty / 6; }
    else {
        vx2 = tx - b->vx; vy2 = ty - b->vy;
        ovl5_sail(b, 0);
        ovl5_draw_ships(b);
        b_delay(b, 0xFA0);
    }
    if (ovl5_input(b)) return 1;
    b->fired_sunk = b->fired_struck = -1;
    bool hidden = (h2 >= 11 && h2 <= 13 && b->crew_side[s] && (b->aim_r_set[s] == 0 || (big && b->aim_r_set[s] == 1))) ||
                  (h2 >= 3 && h2 <= 5 && !b->crew_side[s] && (b->aim_l_set[s] == 0 || (big && b->aim_l_set[s] == 1)));
    if (!shot_hits(b, s, tx, ty, 1) && b->view == 3 && !hidden) splash(b, SO6 + 0x2F5, vy2, vx2, 0);
    b->shot_x[0] = vx2;
    b->shot_y[0] = vy2;
    int shots = 6 - b->hull[s];
    if (!big) shots /= 2;
    if (shots <= 0) shots = 1;
    int spread_y = 22, spread_x = 22;
    if (head_w(b, s, 0) || head_e(b, s, 0)) spread_y = 33; else spread_x = 33;
    if (b->fired_sunk == -1) {
        int n = 1;
        for (int k = 0; k < shots; k++) {
            int dir = b_random(b) < 64 ? -1 : 1;
            b_random(b);
            b->p->rnd++;
            int oy = 0, ox = 0;
            for (int j = 0, t = 0; j < 6; j++, t += spread_y) if (b->p->rnd > t) oy += dir;
            dir = b_random(b) < 64 ? -1 : 1;
            b_random(b);
            b->p->rnd++;
            for (int j = 0, t = 0; j < 6; j++, t += spread_x) if (b->p->rnd > t) ox += dir;
            int hf = b_random(b) <= 64;
            int sx = ox + tx, sy = oy + ty;
            if (b->view == 2) { sx /= 6; sy /= 6; }
            else { sx -= b->vx; sy -= b->vy; }
            if (!shot_hits(b, s, ox + tx, oy + ty, 1)) {
                if (b->view == 3 && !hidden) splash(b, SO6 + 0x2F5, sy, sx, hf);
                else b_delay(b, 200);
            }
            b_click(b);
            b->shot_y[n] = sy; b->shot_x[n] = sx; n++;
            if (b->view == 3) { sx = tx - ox - b->vx; sy = ty - oy - b->vy; }
            if (!shot_hits(b, s, tx - ox, ty - oy, 1)) {
                if (b->view == 3 && !hidden) splash(b, SO6 + 0x2F5, sy, sx, hf);
                else b_delay(b, 200);
            }
            b_click(b);
            b->shot_y[n] = sy; b->shot_x[n] = sx; n++;
            if (!big && k > 2) break;
            if (b->fired_sunk != -1) break;
            if (k == 3) {
                for (int i = 0; i < BS; i++) ovl6_particles(b, i);
                ovl5_draw_ships(b);
            } else b_delay(b, b_random(b) * (b->view == 2 ? 20 : 10));
        }
    }
    if (b->view == 3 && b->fired_sunk == -1) {
        b_delay(b, 0x1B58);
        for (int i = 0; i < BS; i++) ovl6_particles(b, i);
        ovl5_draw_ships(b);
        for (int i = 0; i < BSHOTS; i++)
            if (b->shot_y[i] != NOSHIP && !hidden) g_erase(b, SO6 + 0x2F5, b->shot_y[i], b->shot_x[i], 0);
        for (int i = 0; i < BSHOTS; i++)
            if (b->shot_y[i] != NOSHIP && !hidden) splash(b, SO6 + 0x2FF, b->shot_y[i], b->shot_x[i], 0);
        b_delay(b, 0x1770);
    }
    for (int i = 0; i < BSHOTS; i++) {
        if (b->shot_y[i] != NOSHIP) g_erase(b, BMEM_SO8 + 0x4820, b->shot_y[i], b->shot_x[i], 0);
        b->shot_y[i] = NOSHIP;
    }
    if (s == b->sel) {
        ovl6_crew_panel(b);
        if (b->crew_side[b->sel] || b->demo) {
            b->crew_x--;
            g_panel_sprite(b, PANEL + 0x724, b->crew_x, 0, 0);
            b->aim_r_set[b->sel] = 2;
        } else {
            b->crew_x++;
            g_panel_sprite(b, PANEL + 0x6D2, b->crew_x, 0, 0);
            b->aim_l_set[b->sel] = 2;
        }
    }
    if (b->fired_sunk != -1) {
        for (int guard = 0; guard < BS + 1; guard++) {
            explosion(b);
            bool more = false;
            for (int i = 0; i < BS; i++) {
                if (b->x[i] == NOSHIP || b->hull[i] > 10) continue;
                int lim = (b_random(b) >> 3) + 4;
                if (rt_distance(b, b->fired_sunk, i) >= lim) continue;
                b->hull[i] = 18;
                b->fired_sunk = i;
                more = true;
                break;
            }
            if (!more) break;
        }
    } else if (b->fired_struck != -1) {
        int f = b->fired_struck;
        if (b->owner[f] == 0) ovl6_message(b, 0x2FA, b->name[f], 0, 1);           /* You disabled the X. */
        else {
            ovl6_message(b, 0x30C, b->name[f], 0x311, 0);                         /* The X is disabled. */
            if (f == b->sel && !rt_battle_over(b)) {
                rt_next_ship(b);
                rt_flag_tune(b);
                b_getkey(b);
                b_getkey(b);
            }
        }
        b->flag_tune = false;
    }
    for (int i = 0; i < BS; i++) ovl6_particles(b, i);
    return 0;
}

/* boarding (overlay 7) comes in boarding.c */
